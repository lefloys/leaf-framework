#include "leaf/application/elements/window.hpp"

#include <leaf/core/format.hpp>
#include <leaf/script/math/dim.hpp>
#include <leaf/script/math/pos.hpp>

#include <RmlUi/Core/ComputedValues.h>
#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/Element.h>
#include <RmlUi/Core/ElementDocument.h>
#include <RmlUi/Core/ElementInstancer.h>
#include <RmlUi/Core/Event.h>
#include <RmlUi/Core/EventListener.h>
#include <RmlUi/Core/Factory.h>
#include <RmlUi/Core/Property.h>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <limits>
#include <mutex>
#include <optional>
#include <utility>

namespace lf::detail {
	constexpr bool false_attribute_value(string_view value) {
		return value == "false" ||
			   value == "0" ||
			   value == "disabled" ||
			   value == "no" ||
			   value == "off";
	}

	constexpr f32 default_min_width = 180.0f;
	constexpr f32 default_min_height = 120.0f;
	constexpr f32 unbounded_size = std::numeric_limits<f32>::max() / 8.0f;

	struct ResizeLimits {
		f32 min_width = default_min_width;
		f32 min_height = default_min_height;
		f32 max_width = unbounded_size;
		f32 max_height = unbounded_size;
	};

	string_view default_window_styles() {
		return R"rml(
<style id="window-defaults">
window {
	display: none;
	position: absolute;
	width: 640px;
	min-width: 180px;
	min-height: 120px;
	background: #120A14;
	border-width: 3px;
	border-color: #3B1F2D;
	border-top-color: #C33A4A;
	border-left-color: #3B1F2D;
	border-right-color: #050407;
	border-bottom-color: #050407;
	color: #F4EEF2;
	z-index: 1;
}
window.window-measuring,
window.window-placed {
	display: block;
}
window.window-measuring {
	visibility: hidden;
	pointer-events: none;
}
window.window-focused {
	border-color: #C33A4A;
	border-top-color: #E06A5F;
	border-left-color: #C33A4A;
	border-right-color: #050407;
	border-bottom-color: #050407;
}
window.window-collapsed {
	min-height: 0px;
	max-height: none;
}
window-title {
	display: block;
	height: 36px;
	padding: 8px 86px 0px 16px;
	background: #1D101F;
	border-width: 0px 0px 2px 0px;
	border-bottom-color: #3B1F2D;
	color: #F4EEF2;
	font-size: 18px;
	overflow: hidden;
}
window.window-focused window-title {
	background: #1D101F;
	border-bottom-color: #E06A5F;
	color: #F4EEF2;
}
window-title button {
	position: absolute;
	right: 10px;
	top: 7px;
	width: 28px;
	height: 28px;
	padding: 0px;
	margin: 0px;
	background: #0B070D;
	background-color: #0B070D;
	decorator: none;
	border-width: 3px;
	border-color: #3B1F2D;
	border-top-color: #C33A4A;
	border-left-color: #C33A4A;
	border-right-color: #050407;
	border-bottom-color: #050407;
	color: #F4EEF2;
	font-size: 0px;
	line-height: 0px;
	text-align: center;
}
window-title button:hover {
	background: #5B1F32;
	background-color: #5B1F32;
	border-top-color: #E06A5F;
	border-left-color: #E06A5F;
	color: #F4EEF2;
}
window-title button:active,
window-title button.window-button-pressed {
	background: #E06A5F;
	background-color: #E06A5F;
	border-top-color: #050407;
	border-left-color: #050407;
	border-right-color: #C33A4A;
	border-bottom-color: #F4EEF2;
	color: #050407;
}
window-title button.window-collapse-button {
	right: 45px;
	padding: 0px;
}
window-icon {
	display: block;
	position: absolute;
	left: 4px;
	top: 4px;
	width: 20px;
	height: 20px;
}
window-pixel {
	display: block;
	position: absolute;
	width: 4px;
	height: 4px;
	background: #F4EEF2;
	background-color: #F4EEF2;
}
window-title button:hover window-pixel {
	background: #F4EEF2;
	background-color: #F4EEF2;
}
window-title button:active window-pixel,
window-title button.window-button-pressed window-pixel {
	background: #050407;
	background-color: #050407;
}
window.window-collapsed window-body,
window.window-collapsed window-resize {
	display: none;
}
window-body {
	display: block;
	position: relative;
	margin: 12px;
	overflow: auto;
}
window-resize {
	display: block;
	position: absolute;
	background: transparent;
}
window-resize[direction=se] {
	right: -5px;
	bottom: -5px;
	width: 16px;
	height: 16px;
	background: #C33A4A;
}
window[resizable=false] window-resize,
window[resizable=0] window-resize,
window[resizable=disabled] window-resize,
window[resizable=no] window-resize,
window[resizable=off] window-resize {
	display: none;
}
keybind {
	display: none;
}
</style>
)rml";
	}

	string_view close_icon_rml() {
		return R"rml(<window-icon><window-pixel style="left:4px; top:4px;"></window-pixel><window-pixel style="left:8px; top:8px;"></window-pixel><window-pixel style="left:12px; top:12px;"></window-pixel><window-pixel style="left:12px; top:4px;"></window-pixel><window-pixel style="left:4px; top:12px;"></window-pixel></window-icon>)rml";
	}

	string_view collapse_down_icon_rml() {
		return R"rml(
<window-icon>
	<window-pixel style="left:0px; top:4px;"></window-pixel>
	<window-pixel style="left:4px; top:8px;"></window-pixel>
	<window-pixel style="left:8px; top:12px;"></window-pixel>
	<window-pixel style="left:12px; top:8px;"></window-pixel>
	<window-pixel style="left:16px; top:4px;"></window-pixel>
</window-icon>
)rml";
	}

	string_view collapse_up_icon_rml() {
		return R"rml(
<window-icon>
	<window-pixel style="left:0px; top:12px;"></window-pixel>
	<window-pixel style="left:4px; top:8px;"></window-pixel>
	<window-pixel style="left:8px; top:4px;"></window-pixel>
	<window-pixel style="left:12px; top:8px;"></window-pixel>
	<window-pixel style="left:16px; top:12px;"></window-pixel>
</window-icon>
)rml";
	}

	bool same_tag(const Rml::Element& element, string_view tag) {
		return element.GetTagName() == tag;
	}

	bool false_attribute(const Rml::Element& element, const Rml::String& name) {
		string value = element.GetAttribute<Rml::String>(name, "");
		return false_attribute_value(value);
	}

	bool interactive_element(const Rml::Element& element) {
		const Rml::String& tag = element.GetTagName();
		return tag == "button" ||
			   tag == "input" ||
			   tag == "textarea" ||
			   tag == "select" ||
			   tag == "scrollbarvertical" ||
			   tag == "scrollbarhorizontal" ||
			   tag == "slidertrack" ||
			   tag == "sliderbar" ||
			   tag == "sliderarrowdec" ||
			   tag == "sliderarrowinc";
	}

	Rml::Element* find_ancestor(Rml::Element* element, string_view tag) {
		for (; element; element = element->GetParentNode()) {
			if (same_tag(*element, tag)) {
				return element;
			}
		}
		return nullptr;
	}

	Rml::Element* find_window(Rml::Element* element) {
		return find_ancestor(element, "window");
	}

	Rml::Element* find_child(Rml::Element& element, string_view tag) {
		for (int i = 0; i < element.GetNumChildren(); ++i) {
			Rml::Element* child = element.GetChild(i);
			if (child && same_tag(*child, tag)) {
				return child;
			}
		}
		return nullptr;
	}

	Rml::Element* find_ancestor_class(Rml::Element* element, string_view class_name) {
		for (; element; element = element->GetParentNode()) {
			if (element->IsClassSet(Rml::String(class_name))) {
				return element;
			}
		}
		return nullptr;
	}

	Rml::Element* find_descendant_class(Rml::Element& element, string_view class_name) {
		Rml::ElementList elements;
		element.GetElementsByClassName(elements, Rml::String(class_name));
		return elements.empty() ? nullptr : elements.front();
	}

	bool has_interactive_ancestor(Rml::Element& target) {
		for (Rml::Element* element = &target; element; element = element->GetParentNode()) {
			if (interactive_element(*element)) {
				return true;
			}
		}
		return false;
	}

	void set_px(Rml::Element& element, string_view property, f32 value) {
		element.SetProperty(Rml::String(property), Rml::String(lf::format("{}px", value)));
	}

	std::optional<Rml::String> local_property(Rml::Element& element, string_view property) {
		if (const Rml::Property* value = element.GetLocalProperty(Rml::String(property))) {
			return value->ToString();
		}
		return std::nullopt;
	}

	bool inline_style_has_property(const Rml::Element& element, string_view property) {
		string style = element.GetAttribute<Rml::String>("style", "");
		size_t offset = 0;
		while ((offset = style.find(property, offset)) != string::npos) {
			const size_t end = offset + property.size();
			const bool valid_prefix = offset == 0 ||
									  style[offset - 1] == ';' ||
									  std::isspace(static_cast<unsigned char>(style[offset - 1]));
			size_t cursor = end;
			while (cursor < style.size() && std::isspace(static_cast<unsigned char>(style[cursor]))) {
				++cursor;
			}
			if (valid_prefix && cursor < style.size() && style[cursor] == ':') {
				return true;
			}
			offset = end;
		}
		return false;
	}

	bool authored_position_property(const Rml::Element& element, string_view property) {
		if (inline_style_has_property(element, property)) {
			return true;
		}

		const Rml::ComputedValues& values = element.GetComputedValues();
		if (property == "left") {
			return values.left().type != Rml::Style::LengthPercentageAuto::Auto;
		}
		if (property == "top") {
			return values.top().type != Rml::Style::LengthPercentageAuto::Auto;
		}
		return false;
	}

	string placement_value(const Rml::Element& element) {
		string placement = element.GetAttribute<Rml::String>("placement", "");
		std::transform(placement.begin(), placement.end(), placement.begin(), [](unsigned char ch) {
			return static_cast<char>(std::tolower(ch));
		});
		return placement;
	}

	bool manual_placement(const Rml::Element& element) {
		return placement_value(element) == "manual" ||
			   authored_position_property(element, "left") ||
			   authored_position_property(element, "top");
	}

	bool center_placement(const Rml::Element& element) {
		return placement_value(element) == "center";
	}

	void reveal(Rml::Element& element) {
		element.SetClass("window-measuring", false);
		element.SetClass("window-placed", true);
	}

	i32 next_window_z_index() {
		// Window focus is applied during the capture phase of mousedown. Keep
		// focused windows above authored HUD/overlay layers; starting at 10
		// demoted hosts such as z-index:15000 between mouse-down and mouse-up,
		// changing the hit target and preventing RmlUi from emitting click.
		static i32 next_z = 100000;
		return next_z++;
	}

	void restore_local_property(Rml::Element& element, string_view property, const std::optional<Rml::String>& value) {
		Rml::String property_name(property);
		if (value) {
			element.SetProperty(property_name, *value);
		} else {
			element.RemoveProperty(property_name);
		}
	}

	void set_string_property(Rml::Element& element, string_view property, string_view value) {
		element.SetProperty(Rml::String(property), Rml::String(value));
	}

	void style_chrome_button(Rml::Element& button, f32 right) {
		set_string_property(button, "position", "absolute");
		set_px(button, "right", right);
		set_px(button, "top", 7.0f);
		set_px(button, "width", 28.0f);
		set_px(button, "height", 28.0f);
		set_px(button, "margin", 0.0f);
		set_px(button, "padding", 0.0f);
		set_px(button, "font-size", 0.0f);
		set_px(button, "line-height", 0.0f);
		set_string_property(button, "text-align", "center");
	}

	f32 clamp_range(f32 value, f32 low, f32 high) {
		return std::clamp(value, low, std::max(low, high));
	}

	void descendant_extent(Rml::Element& root, Rml::Element& element, f32& width, f32& height) {
		for (int i = 0; i < element.GetNumChildren(); ++i) {
			if (Rml::Element* child = element.GetChild(i)) {
				if (same_tag(*child, "scrollbarvertical") || same_tag(*child, "scrollbarhorizontal")) {
					continue;
				}
				const Rml::Box& box = child->GetBox();
				const Rml::Vector2f margin_position = box.GetPosition(Rml::BoxArea::Margin);
				const Rml::Vector2f margin_size = box.GetSize(Rml::BoxArea::Margin);
				const f32 child_left = child->GetAbsoluteLeft() - root.GetAbsoluteLeft() + margin_position.x;
				const f32 child_top = child->GetAbsoluteTop() - root.GetAbsoluteTop() + margin_position.y;
				width = std::max(width, child_left + margin_size.x);
				height = std::max(height, child_top + margin_size.y);
				descendant_extent(root, *child, width, height);
			}
		}
	}

	f32 child_extent_width(Rml::Element& element) {
		f32 width = 0.0f;
		f32 height = 0.0f;
		descendant_extent(element, element, width, height);
		return width;
	}

	f32 child_extent_height(Rml::Element& element) {
		f32 width = 0.0f;
		f32 height = 0.0f;
		descendant_extent(element, element, width, height);
		return height;
	}

	f32 width_outside_content(Rml::Element& element) {
		return std::max(0.0f, element.GetOffsetWidth() - element.GetBox().GetSize(Rml::BoxArea::Content).x);
	}

	f32 height_outside_content(Rml::Element& element) {
		return std::max(0.0f, element.GetOffsetHeight() - element.GetBox().GetSize(Rml::BoxArea::Content).y);
	}

	f32 margin_width_outside_content(Rml::Element& element) {
		return std::max(0.0f, element.GetBox().GetSize(Rml::BoxArea::Margin).x - element.GetBox().GetSize(Rml::BoxArea::Content).x);
	}

	f32 margin_height_outside_content(Rml::Element& element) {
		return std::max(0.0f, element.GetBox().GetSize(Rml::BoxArea::Margin).y - element.GetBox().GetSize(Rml::BoxArea::Content).y);
	}

	f32 max_size_with_chrome(f32 max_content_size, f32 chrome_size) {
		if (max_content_size >= unbounded_size) {
			return max_content_size;
		}
		return max_content_size + chrome_size;
	}

	ResizeLimits resize_limits(Rml::Element& element) {
		Rml::Vector2f containing_block = element.GetContainingBlock();
		const Rml::ComputedValues& values = element.GetComputedValues();
		const f32 width_extra = width_outside_content(element);
		const f32 height_extra = height_outside_content(element);

		const f32 min_width = Rml::ResolveValueOr(values.min_width(), containing_block.x, default_min_width);
		const f32 min_height = Rml::ResolveValueOr(values.min_height(), containing_block.y, default_min_height);
		const f32 max_width = Rml::ResolveValueOr(values.max_width(), containing_block.x, unbounded_size);
		const f32 max_height = Rml::ResolveValueOr(values.max_height(), containing_block.y, unbounded_size);

		ResizeLimits limits;
		limits.min_width = std::max(0.0f, min_width + width_extra);
		limits.min_height = std::max(0.0f, min_height + height_extra);
		limits.max_width = std::max(limits.min_width, max_size_with_chrome(max_width, width_extra));
		limits.max_height = std::max(limits.min_height, max_size_with_chrome(max_height, height_extra));
		return limits;
	}

	bool is_title_drag_target(Rml::Element& target) {
		if (has_interactive_ancestor(target)) {
			return false;
		}
		if (target.HasAttribute("window-drag")) {
			return true;
		}
		return same_tag(target, "window-title") || find_ancestor(target.GetParentNode(), "window-title");
	}

	void set_collapse_label(Rml::Element& window, bool collapsed) {
		Rml::Element* title = find_child(window, "window-title");
		if (!title) {
			return;
		}
		Rml::Element* button = find_descendant_class(*title, "window-collapse-button");
		if (!button) {
			return;
		}

		button->SetInnerRML(Rml::String(collapsed ? collapse_up_icon_rml() : collapse_down_icon_rml()));
	}

	class ElementWindow final : public Rml::Element, public Rml::EventListener {
	  public:
		explicit ElementWindow(const Rml::String& tag);
		~ElementWindow() override;

		void ProcessEvent(Rml::Event& event) override;
		void cancel_document_interaction();

	  protected:
		void OnChildAdd(Rml::Element* child) override;
		void OnLayout() override;
		void OnUpdate() override;

	  private:
		enum class Operation {
			Drag,
			Resize,
		};

		struct Interaction {
			Operation operation;
			pos2<f32> grab{};
			f32 start_left = 0.0f;
			f32 start_top = 0.0f;
			f32 start_width = 0.0f;
			f32 start_height = 0.0f;
		};

		struct ExpandedLayout {
			f32 height = 0.0f;
			std::optional<Rml::String> min_height;
			std::optional<Rml::String> max_height;
		};

		std::optional<Interaction> interaction;
		Rml::ElementDocument* bound_document = nullptr;
		std::optional<pos2<f32>> normalized_position;
		std::optional<dim2<i32>> normalized_context_size;
		std::optional<ExpandedLayout> expanded_layout;
		bool manually_resized = false;
		bool default_position_applied = false;
		bool initially_raised = false;

		void prepare_for_layout();
		void install_chrome();
		void install_handles();
		bool closable() const;
		void fit_to_content();

		void raise_when_visible();
		void apply_default_position();
		dim2<f32> context_dimensions() const;
		f32 available_width(f32 context_width);
		f32 available_height(f32 context_height);
		void update_normalized_position();
		void maintain_normalized_position();
		void clamp_to_context();
		void set_window_position(f32 left, f32 top);
		void materialize_center_placement();

		pos2<f32> event_position(Rml::Event& event) const;
		void begin(Rml::Event& event);
		void click(Rml::Event& event);
		void move(Rml::Event& event);
		void end();

		void begin_drag_or_resize(Rml::Element& target, pos2<f32> mouse);
		void toggle_collapse(Rml::Element& collapse);
		void apply_drag(pos2<f32> mouse);
		void apply_resize(pos2<f32> mouse);
		void clear_pressed_buttons();
		void focus();
	};

	ElementWindow::ElementWindow(const Rml::String& tag) : Rml::Element(tag) {
		SetAttribute("tabindex", "0");
		SetClass("window-measuring", true);
		AddEventListener("mousedown", this, true);
		AddEventListener("click", this, true);
	}

	ElementWindow::~ElementWindow() {
		if (bound_document) {
			bound_document->RemoveEventListener("mousemove", this, true);
			bound_document->RemoveEventListener("mouseup", this, true);
		}
		RemoveEventListener("mousedown", this, true);
		RemoveEventListener("click", this, true);
	}

	void ElementWindow::ProcessEvent(Rml::Event& event) {
		if (event == "mousedown") {
			begin(event);
		} else if (event == "mousemove") {
			move(event);
		} else if (event == "click") {
			click(event);
		} else if (event == "mouseup") {
			end();
		}
	}

	void ElementWindow::OnChildAdd(Rml::Element* child) {
		Rml::Element::OnChildAdd(child);
		if (!bound_document && GetOwnerDocument()) {
			bound_document = GetOwnerDocument();
			bound_document->AddEventListener("mousemove", this, true);
			bound_document->AddEventListener("mouseup", this, true);
		}
		if (child && !same_tag(*child, "window-resize") && !interaction && !manual_placement(*this)) {
			default_position_applied = false;
			SetClass("window-placed", false);
			normalized_position.reset();
			normalized_context_size.reset();
		}
		prepare_for_layout();
		install_chrome();
		install_handles();
	}

	void ElementWindow::OnLayout() {
		Rml::Element::OnLayout();
		install_chrome();
		install_handles();
		fit_to_content();
		if (!interaction) {
			apply_default_position();
			if (default_position_applied) {
				raise_when_visible();
			}
		}
	}

	void ElementWindow::OnUpdate() {
		Rml::Element::OnUpdate();
		prepare_for_layout();
		install_chrome();
		install_handles();
		if (!interaction) {
			if (center_placement(*this)) {
				apply_default_position();
				return;
			}
			if (normalized_position) {
				maintain_normalized_position();
			} else {
				update_normalized_position();
			}
		}
	}

	void ElementWindow::prepare_for_layout() {
		if (IsClassSet("window-placed") || IsClassSet("window-measuring")) {
			return;
		}
		SetClass("window-measuring", true);
	}

	void ElementWindow::install_chrome() {
		Rml::Element* title = find_child(*this, "window-title");
		if (!title) {
			return;
		}
		title->SetAttribute("window-drag", "");

		if (closable() && !find_descendant_class(*title, "window-close-button")) {
			Rml::ElementPtr close = Rml::Factory::InstanceElement(title, "*", "button", Rml::XMLAttributes());
			if (close) {
				close->SetClass("window-close-button", true);
				style_chrome_button(*close, 10.0f);
				close->SetInnerRML(Rml::String(close_icon_rml()));
				title->AppendChild(std::move(close), true);
			}
		}

		if (!false_attribute(*this, "collapsible") && !find_descendant_class(*title, "window-collapse-button")) {
			Rml::ElementPtr collapse = Rml::Factory::InstanceElement(title, "*", "button", Rml::XMLAttributes());
			if (collapse) {
				collapse->SetClass("window-collapse-button", true);
				style_chrome_button(*collapse, 45.0f);
				collapse->SetInnerRML(Rml::String(collapse_down_icon_rml()));
				title->AppendChild(std::move(collapse), true);
			}
		}
	}

	bool ElementWindow::closable() const {
		return !false_attribute(*this, "closable") && !false_attribute(*this, "window-close");
	}

	void ElementWindow::fit_to_content() {
		if (manually_resized || interaction || IsClassSet("window-collapsed")) {
			return;
		}

		Rml::Element* title = find_child(*this, "window-title");
		Rml::Element* body = find_child(*this, "window-body");
		if (!title || !body || body->GetNumChildren() == 0) {
			return;
		}

		const f32 body_width = child_extent_width(*body);
		const f32 body_height = child_extent_height(*body);
		if (body_width <= 0.0f || body_height <= 0.0f) {
			return;
		}

		const f32 content_width = body_width + margin_width_outside_content(*body);
		const f32 content_height = title->GetOffsetHeight() + body_height + margin_height_outside_content(*body);
		const ResizeLimits limits = resize_limits(*this);
		const f32 width_extra = width_outside_content(*this);
		const f32 height_extra = height_outside_content(*this);
		const f32 width = clamp_range(content_width + width_extra, limits.min_width, limits.max_width);
		const f32 height = clamp_range(content_height + height_extra, limits.min_height, limits.max_height);
		// Authored dimensions are the requested minimum size, not permission for
		// content to live outside the window's interactive box. Never shrink an
		// authored dimension, but grow it when the laid-out children need more
		// room. Otherwise visible overflow can fall outside hit testing.
		const bool preserve_width = inline_style_has_property(*this, "width");
		const bool preserve_height = inline_style_has_property(*this, "height");

		if ((!preserve_width || width > GetOffsetWidth() + 0.5f) && std::abs(width - GetOffsetWidth()) > 0.5f) {
			set_px(*this, "width", std::max(0.0f, width - width_extra));
		}
		if ((!preserve_height || height > GetOffsetHeight() + 0.5f) && std::abs(height - GetOffsetHeight()) > 0.5f) {
			set_px(*this, "height", std::max(0.0f, height - height_extra));
		}
	}

	void ElementWindow::install_handles() {
		if (find_child(*this, "window-resize")) {
			return;
		}

		Rml::ElementPtr handle = Rml::Factory::InstanceElement(this, "*", "window-resize", Rml::XMLAttributes());
		if (!handle) {
			return;
		}
		handle->SetAttribute("direction", "se");
		handle->SetClass("window-resize", true);
		AppendChild(std::move(handle), true);
	}

	void ElementWindow::raise_when_visible() {
		if (initially_raised || GetOffsetWidth() <= 0.0f || GetOffsetHeight() <= 0.0f) {
			return;
		}

		initially_raised = true;
		focus();
	}

	void ElementWindow::apply_default_position() {
		const dim2<f32> context_size = context_dimensions();
		const f32 context_width = context_size.width;
		const f32 context_height = context_size.height;
		const bool should_center = center_placement(*this);

		if (should_center) {
			if (context_width > 0.0f && context_height > 0.0f &&
				GetOffsetWidth() > 0.0f && GetOffsetHeight() > 0.0f) {
				const f32 left = std::max(8.0f, (context_width - GetOffsetWidth()) * 0.5f);
				const f32 top = std::max(8.0f, (context_height - GetOffsetHeight()) * 0.5f);
				set_px(*this, "left", left);
				set_px(*this, "top", top);
			}
			default_position_applied = true;
			reveal(*this);
			return;
		}

		if (default_position_applied) {
			reveal(*this);
			return;
		}
		if (manual_placement(*this)) {
			default_position_applied = true;
			reveal(*this);
			return;
		}

		if (context_width <= 0.0f || context_height <= 0.0f ||
			GetOffsetWidth() <= 0.0f || GetOffsetHeight() <= 0.0f) {
			return;
		}

		const f32 left = std::max(8.0f, (context_width - GetOffsetWidth()) * 0.5f);
		const f32 top = std::max(8.0f, (context_height - GetOffsetHeight()) * 0.5f);
		set_px(*this, "left", left);
		set_px(*this, "top", top);
		default_position_applied = true;
		update_normalized_position();
		reveal(*this);
	}

	dim2<f32> ElementWindow::context_dimensions() const {
		if (!GetContext()) {
			return {};
		}

		const Rml::Vector2i context_size = GetContext()->GetDimensions();
		return {
			.width = std::max(0.0f, static_cast<f32>(context_size.x)),
			.height = std::max(0.0f, static_cast<f32>(context_size.y))
		};
	}

	f32 ElementWindow::available_width(f32 context_width) {
		return std::max(1.0f, context_width - GetOffsetWidth());
	}

	f32 ElementWindow::available_height(f32 context_height) {
		return std::max(1.0f, context_height - GetOffsetHeight());
	}

	void ElementWindow::update_normalized_position() {
		const dim2<f32> context_size = context_dimensions();
		const f32 context_width = context_size.width;
		const f32 context_height = context_size.height;
		if (context_width <= 0.0f || context_height <= 0.0f) {
			return;
		}

		normalized_position = pos2<f32>{
			GetOffsetLeft() / available_width(context_width),
			GetOffsetTop() / available_height(context_height),
		};
		if (GetContext()) {
			Rml::Vector2i size = GetContext()->GetDimensions();
			normalized_context_size = dim2<i32>{ size.x, size.y };
		}
	}

	void ElementWindow::maintain_normalized_position() {
		if (!normalized_position) {
			return;
		}

		const dim2<f32> context_size = context_dimensions();
		const f32 context_width = context_size.width;
		const f32 context_height = context_size.height;
		if (context_width <= 0.0f || context_height <= 0.0f) {
			return;
		}

		const Rml::Vector2i current_context_size = GetContext()->GetDimensions();
		if (!normalized_context_size ||
				normalized_context_size->width != current_context_size.x ||
				normalized_context_size->height != current_context_size.y) {
			set_px(*this, "left", normalized_position->x * available_width(context_width));
			set_px(*this, "top", normalized_position->y * available_height(context_height));
			normalized_context_size = dim2<i32>{ current_context_size.x, current_context_size.y };
		}
		clamp_to_context();
	}

	void ElementWindow::clamp_to_context() {
		const dim2<f32> context_size = context_dimensions();
		const f32 context_width = context_size.width;
		const f32 context_height = context_size.height;
		if (context_width <= 0.0f || context_height <= 0.0f) {
			return;
		}
		const f32 left = clamp_range(GetOffsetLeft(), 8.0f, std::max(8.0f, context_width - GetOffsetWidth() - 8.0f));
		const f32 top = clamp_range(GetOffsetTop(), 8.0f, std::max(8.0f, context_height - GetOffsetHeight() - 8.0f));
		if (std::abs(left - GetOffsetLeft()) > 0.5f || std::abs(top - GetOffsetTop()) > 0.5f) {
			set_px(*this, "left", left);
			set_px(*this, "top", top);
			update_normalized_position();
		}
	}

	void ElementWindow::set_window_position(f32 left, f32 top) {
		set_px(*this, "left", left);
		set_px(*this, "top", top);
		update_normalized_position();
	}

	void ElementWindow::materialize_center_placement() {
		if (!center_placement(*this)) {
			return;
		}
		Rml::Element* offset_parent = GetOffsetParent();
		if (!offset_parent) {
			offset_parent = GetParentNode();
		}
		const f32 parent_left = offset_parent ? offset_parent->GetAbsoluteLeft() : 0.0f;
		const f32 parent_top = offset_parent ? offset_parent->GetAbsoluteTop() : 0.0f;
		const f32 left = GetAbsoluteLeft() - parent_left;
		const f32 top = GetAbsoluteTop() - parent_top;
		set_string_property(*this, "position", "absolute");
		set_px(*this, "left", left);
		set_px(*this, "top", top);
		SetOffset(Rml::Vector2f(left, top), offset_parent);
		RemoveAttribute("placement");
		update_normalized_position();
	}

	pos2<f32> ElementWindow::event_position(Rml::Event& event) const {
		return {
			event.GetParameter<f32>("mouse_x", 0.0f),
			event.GetParameter<f32>("mouse_y", 0.0f),
		};
	}

	void ElementWindow::begin(Rml::Event& event) {
		if (event.GetParameter<int>("button", -1) != 0) {
			return;
		}
		end();

		Rml::Element* target = event.GetTargetElement();
		if (!target || find_window(target) != this) {
			return;
		}

		install_chrome();
		install_handles();
		focus();
		materialize_center_placement();

		if (Rml::Element* close = find_ancestor_class(target, "window-close-button")) {
			if (find_window(close) == this) {
				close->SetClass("window-button-pressed", true);
				return;
			}
		}
		if (Rml::Element* collapse = find_ancestor_class(target, "window-collapse-button")) {
			if (find_window(collapse) == this) {
				collapse->SetClass("window-button-pressed", true);
				event.StopImmediatePropagation();
				return;
			}
		}

		begin_drag_or_resize(*target, event_position(event));
		if (!interaction) {
			return;
		}

		event.StopImmediatePropagation();
	}

	void ElementWindow::begin_drag_or_resize(Rml::Element& target, pos2<f32> mouse) {
		auto begin_interaction = [&](Operation operation) {
			interaction = Interaction{
				operation,
				mouse,
				GetOffsetLeft(),
				GetOffsetTop(),
				GetOffsetWidth(),
				GetOffsetHeight(),
			};
		};

		if (IsClassSet("window-collapsed")) {
			if (is_title_drag_target(target)) {
				begin_interaction(Operation::Drag);
			}
		} else if (!false_attribute(*this, "resizable")) {
			if (Rml::Element* resize = find_ancestor(&target, "window-resize")) {
				if (resize->GetAttribute<Rml::String>("direction", "") == "se") {
					begin_interaction(Operation::Resize);
				}
			}
		}

		if (!interaction && is_title_drag_target(target)) {
			begin_interaction(Operation::Drag);
		}

		if (interaction) {
			update_normalized_position();
		}
	}

	void ElementWindow::click(Rml::Event& event) {
		Rml::Element* target = event.GetTargetElement();
		if (Rml::Element* close = find_ancestor_class(target, "window-close-button")) {
			if (find_window(close) == this) {
				if (HasAttribute("window-close") && !false_attribute(*this, "window-close")) {
					DispatchEvent("window-close", Rml::Dictionary());
				} else {
					SetProperty("display", "none");
				}
				event.StopImmediatePropagation();
				return;
			}
		}

		Rml::Element* collapse = find_ancestor_class(target, "window-collapse-button");
		if (collapse && find_window(collapse) == this) {
			toggle_collapse(*collapse);
			event.StopImmediatePropagation();
		}
	}

	void ElementWindow::toggle_collapse(Rml::Element& collapse) {
		install_chrome();
		install_handles();
		focus();

		const f32 left = GetOffsetLeft();
		const f32 top = GetOffsetTop();

		if (IsClassSet("window-collapsed")) {
			SetClass("window-collapsed", false);
			if (expanded_layout) {
				set_px(*this, "height", expanded_layout->height);
				restore_local_property(*this, "min-height", expanded_layout->min_height);
				restore_local_property(*this, "max-height", expanded_layout->max_height);
				expanded_layout.reset();
			}
			set_collapse_label(*this, false);
		} else {
			expanded_layout = ExpandedLayout{
				GetBox().GetSize(Rml::BoxArea::Content).y,
				local_property(*this, "min-height"),
				local_property(*this, "max-height"),
			};
			SetClass("window-collapsed", true);
			SetProperty("min-height", "0px");
			SetProperty("max-height", "none");
			if (Rml::Element* title = find_ancestor(&collapse, "window-title")) {
				set_px(*this, "height", title->GetOffsetHeight());
			}
			set_collapse_label(*this, true);
		}

		set_px(*this, "left", left);
		set_px(*this, "top", top);
		normalized_position.reset();
	}

	void ElementWindow::move(Rml::Event& event) {
		if (!interaction) {
			return;
		}

		pos2<f32> mouse = event_position(event);
		if (interaction->operation == Operation::Drag) {
			apply_drag(mouse);
		} else {
			apply_resize(mouse);
		}

		event.StopImmediatePropagation();
	}

	void ElementWindow::apply_drag(pos2<f32> mouse) {
		set_window_position(interaction->start_left + mouse.x - interaction->grab.x, interaction->start_top + mouse.y - interaction->grab.y);
	}

	void ElementWindow::apply_resize(pos2<f32> mouse) {
		const f32 delta_x = mouse.x - interaction->grab.x;
		const f32 delta_y = mouse.y - interaction->grab.y;
		const f32 width_extra = width_outside_content(*this);
		const f32 height_extra = height_outside_content(*this);
		const ResizeLimits limits = resize_limits(*this);

		const f32 width = clamp_range(interaction->start_width + delta_x, limits.min_width, limits.max_width);
		const f32 height = clamp_range(interaction->start_height + delta_y, limits.min_height, limits.max_height);

		set_px(*this, "width", std::max(0.0f, width - width_extra));
		set_px(*this, "height", std::max(0.0f, height - height_extra));
		manually_resized = true;
		clamp_to_context();
		update_normalized_position();
		DispatchEvent("window-resize", Rml::Dictionary());
	}

	void ElementWindow::end() {
		clear_pressed_buttons();
		interaction.reset();
	}

	void ElementWindow::cancel_document_interaction() {
		end();
	}

	void ElementWindow::clear_pressed_buttons() {
		Rml::ElementList buttons;
		GetElementsByClassName(buttons, "window-button-pressed");
		for (Rml::Element* button : buttons) {
			button->SetClass("window-button-pressed", false);
		}
	}

	void ElementWindow::focus() {
		// Do NOT call Focus(true) here. This runs during the mousedown
		// capture-phase listener installed on the window, before the event
		// reaches the actual click target. Stealing keyboard focus to the
		// window robs every form control (input, textarea, select) inside
		// it of focus, so typing goes nowhere and clicking a second input
		// never visually unfocuses the first. The window only needs z-order
		// raising and the "window-focused" class for styling — neither of
		// those requires the window itself to be the focused element.
		const i32 z_index = next_window_z_index();
		if (Rml::ElementDocument* document = GetOwnerDocument()) {
			Rml::ElementList windows;
			document->GetElementsByTagName(windows, "window");
			for (Rml::Element* item : windows) {
				item->SetClass("window-focused", item == this);
			}

			for (Rml::Element* parent = GetParentNode(); parent && parent != document; parent = parent->GetParentNode()) {
				parent->SetProperty("z-index", Rml::String(lf::format("{}", z_index)));
			}
		}

		SetProperty("z-index", Rml::String(lf::format("{}", z_index)));
	}
} // namespace lf::detail

namespace lf {
	string install_window_defaults(string_view source) {
		constexpr string_view marker = "window-defaults";
		if (source.find(marker) != string_view::npos) {
			return string(source);
		}

		constexpr string_view head_open = "<head>";
		size_t insert_at = source.find(head_open);
		if (insert_at != string_view::npos) {
			insert_at += head_open.size();
		} else {
			insert_at = source.find("</head>");
		}
		if (insert_at == string_view::npos) {
			return string(detail::default_window_styles()) + string(source);
		}

		string out;
		out.reserve(source.size() + detail::default_window_styles().size());
		out += source.substr(0, insert_at);
		out += detail::default_window_styles();
		out += source.substr(insert_at);
		return out;
	}

	void RegisterWindowElement() {
		static Rml::ElementInstancerGeneric<detail::ElementWindow> instancer;
		static std::once_flag registered;
		std::call_once(registered, [] {
			Rml::Factory::RegisterElementInstancer("window", &instancer);
		});
	}

	void ReleaseWindowDocumentEvents(Rml::ElementDocument& document) {
		Rml::ElementList windows;
		document.GetElementsByTagName(windows, "window");
		for (Rml::Element* element : windows) {
			if (auto* window = rmlui_dynamic_cast<detail::ElementWindow*>(element)) {
				window->cancel_document_interaction();
			}
		}
	}
} // namespace lf
