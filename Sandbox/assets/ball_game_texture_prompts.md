# Gravity Golf Texture Prompts

Use these prompts with your preferred image generator. Aim for clean, readable game assets; avoid text in images. Resolution suggestions provided; export with transparent background (PNG) for sprites and seamless tiling for terrain.

## 1) Grass Background (Tiled)
- Prompt: "Top-down stylized grass tile, subtle noise and soft color variation, gentle highlights, no flowers, seamless tiling, 1024x1024"
- Notes: Keep contrast low so gameplay elements pop. Export two variants (A/B) with the same pattern but B is ~10-12% darker so tiles can alternate for a subtle checker variation.

## 2) Golf Ball
- Prompt: "Minimalist glossy white golf ball, subtle dimples, soft top-down shading, tiny highlight, transparent background, 512x512"

## 3) Goal Hole
- Prompt: "Stylized golf hole ring, gold outer ring, dark inner hole, slight bevel and shadow, top-down view, transparent background, 512x512"

## 4) Walls (Stone/Concrete)
- Prompt: "Top-down stone barrier segment, slightly beveled edges, cool gray tones, light edge highlight and soft shadow, tileable horizontally, transparent background, 512x256"
- Variant: warm sandstone colorway for optional theme swap.

## 5) Bounce / Booster Pad
- Prompt: "Top-down futuristic booster pad, glowing arrow center pointing along +X, bright orange accents, metallic rim, subtle emission, transparent background, 512x512"
- Variant: "Top-down futuristic booster pad, blue/cyan energy arrow, metallic rim, subtle emission, transparent background, 512x512"

## 6) Gravity Well (Optional future)
- Prompt: "Top-down circular energy well, swirling pattern, emissive blue core with faint outer glow, transparent background, 512x512"
- Repulsor variant: swap blue to orange/red.

## 7) UI Icons (Minimal)
- Prompt: "Flat minimalist UI icon set: restart arrow, star, speaker on/off, mouse click, in a single sheet on transparent background, 1024x1024; clean outlines, consistent stroke weight"

## 8) Game-Ready Sound Prompts
- Swing/shot: "Concise whoosh with a crisp start and short tail, soft high-frequency air, no wind rumble, under 0.4s, normalized"
- Ball rolling: "Soft granular rolling loop on grass, gentle mid-frequency texture, no clicks, seamless 0.8s loop"
- Wall impact: "Short woody thud with light click, slight low-mid thump, under 0.25s, no reverb"
- Cup capture: "Pleasant chime with 2-3 harmonic overtones, fast attack, 0.6s decay, bright but not piercing"
- Booster pad: "Quick rising synth whoosh with a soft shimmer tail, under 0.5s, stereo-friendly"
- Gravity well: "Subtle swirling airy pad with slow LFO movement, 1s loopable bed, no harsh highs"
- UI click: "Tight click-pop with soft tick body, under 0.15s, no reverb"
- UI confirm: "Two-tone pluck, gentle attack, 0.35s decay, friendly and clean"
- Ambience (optional): "Light outdoor ambience with faint breeze, no birds/voices, loopable 15s"

## Color & Style Targets
- Grass: #0b1d14 base, subtle variations within ±10% brightness.
- Walls: #3b3f48 with lighter edge highlight.
- Goal ring: gold (#d9a432), inner hole nearly black.
- Ball: soft white (#f5f6fa) with faint gray shadow.

## Export Checklist
- PNG with transparency (except grass which can be seamless tile).
- Keep margins tight; center the subject.
- Avoid text, logos, or gradients that break tiling.
- Provide power-of-two sizes where possible (256, 512, 1024).
