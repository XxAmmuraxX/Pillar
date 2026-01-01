# Pillar Engine – Missing Convenience APIs (Complaints)

While wiring the Gravity Golf particles/wells, a few painful gaps showed up that the engine could cover with small API additions. These would have avoided the compile errors about `DrawQuad` overloads and vector ops.

## Assets / resource workflow
- Add an engine-level resource cache (e.g., `AssetCache<Texture2D>` / `AssetCache<AudioBuffer>`) keyed by path so repeated loads don’t re-hit disk and create duplicate GPU/audio objects.
- Add `AssetManager::Exists(path)` and `AssetManager::ReadFileBytes/ReadFileString` helpers so gameplay systems can load data without re-implementing filesystem handling.
- Provide an opt-in file watching / hot reload hook (textures, animations, shaders) since iteration speed matters a lot when actually building a game.

## Physics query helpers
- Physics exposes raw `b2World*` (good), but add convenience wrappers for common gameplay queries: `Raycast2D`, `CircleCast`, `OverlapBox`, and “query all fixtures in AABB” with clean callbacks.
- Add contact/query helpers that return `Entity` (or UUID) instead of raw Box2D pointers, to reduce glue code between ECS and physics.
