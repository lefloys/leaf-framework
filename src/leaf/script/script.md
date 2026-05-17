Mods are global per process, not per instance of the game. Multiple game instances running through
the same process share the same mods, which can lead to unexpected behavior if it is not handled
explicitly.

1. Setting stage: mods are collected and sorted into a dependency graph. Setting prototypes are
   registered.
2. Data stage: each mod runs its data stage, where prototypes are created and registered.
3. Runtime stage: the modding environment is fully set up.
