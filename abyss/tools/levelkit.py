"""Shared helpers for the level authoring scripts. Legend:
  # solid   = one-way ledge   ^ spike   H ladder   $ coin   C chest   E exit   S spawn   x crumbling floor   R rune   G gate
  T torch (decor)  z skull (decor)  k crawler  m moving platform (surface = top of its tile)
  L lava (hurts 30)  V fire vent (solid floor tile, flame column above)  t timed platform (solid part of the time)
  P teleport pad (pads link 1st<->2nd, 3rd<->4th ... in row-major order)  g gravity-flip zone (non-solid field)"""
def mk(W, ops):
    g = [['.'] * W for _ in range(15)]
    for x in range(W): g[0][x] = '#'; g[13][x] = '#'; g[14][x] = '#'
    for y in range(15): g[y][0] = '#'; g[y][W - 1] = '#'
    for ch, x0, x1, y0, y1 in ops:
        for y in range(y0, y1 + 1):
            for x in range(x0, x1 + 1): g[y][x] = ch
    return g
def save(world, fn, name, W, ops, boss=0, rain=False):
    g = mk(W, ops)
    hdr = f"name: {name}\nworld: {world}\n" + (f"boss: {boss}\n" if boss else "") + ("rain: 1\n" if rain else "")
    import os; os.makedirs(f'levels/world{world}', exist_ok=True)
    open(f'levels/world{world}/{fn}.txt', 'w').write(hdr + "\n".join("".join(r) for r in g) + "\n")
