# Pillar Engine – Missing Convenience APIs (Complaints)

While wiring the Gravity Golf particles/wells, a few painful gaps showed up that the engine could cover with small API additions. These would have avoided the compile errors about `DrawQuad` overloads and vector ops.

## Renderer2DBackend gaps
- Add `DrawQuad` overloads that accept `glm::vec3 position` + `glm::vec2 size` + `glm::vec4 color` **without** requiring a texture; today only the vec2 version exists, so z-layered colored quads force extra parameters or fail overload resolution.
- Add `DrawQuad` overloads that accept `glm::vec3 position` + `glm::vec2 size` + `std::shared_ptr<Texture2D>` with default white tint; current signature also needs an explicit color argument which is awkward when you just want textured output.
- Provide rotated variants that accept `glm::vec3 position` (z-layer), UV ranges, and tint in one call; today the rotated textured overload only supports `glm::vec2 position` and no UV control, which blocks layered, UV-clipped effects.
- Consider a single flexible signature (e.g., struct of draw params) to reduce combinatorial overloads and avoid “too few arguments” errors when adding new visuals.
- Renderer2D needs an easy way to render a 2D pass without writing depth (or a simple state guard) so alpha textures don’t punch out depth and occlude later sprites; we currently have to call OpenGL depth toggles manually in client code.

## Math and utility helpers
- Offer lightweight helpers for 2D vector ops used constantly in gameplay code (e.g., safe normalize with epsilon, clamp-length, lerp), instead of re-implementing or hitting operator mismatches; this would prevent “built-in operator '+' cannot be applied” style errors when mixing glm types or literals.
- Provide a small RNG/Random utility (float range, angle) in the engine core to avoid sprinkling `std::rand()` and custom wrappers across layers; particles and gameplay effects would become simpler and less error-prone.

These additions would make rendering/FX code more plug-and-play and prevent the current overload/syntax errors when using z-layered quads, UVs, and simple vector math in gameplay layers.
