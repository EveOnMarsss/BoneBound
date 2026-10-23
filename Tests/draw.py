import sys, struct
from PIL import Image
dump, out, scale = sys.argv[1], sys.argv[2], int(sys.argv[3])
ts = Image.open('/home/claude/work/BoneBound/resources/tileset/DB_curses_12x12.png').convert('RGB')
glyphs = {}
def glyph(i):
    if i in glyphs: return glyphs[i]
    cx, cy = (i % 16) * 12, (i // 16) * 12
    g = Image.new('L', (12, 12))
    px = g.load()
    for y in range(12):
        for x in range(12):
            px[x, y] = ts.getpixel((cx + x, cy + y))[1]
    g = g.resize((scale, scale), Image.NEAREST)
    glyphs[i] = g; return g
d = open(dump, 'rb').read()
w, h = struct.unpack('ii', d[:8]); d = d[8:]
img = Image.new('RGB', (w * scale, h * scale), (0, 0, 0))
for j in range(h):
    for i in range(w):
        b = d[(j * w + i) * 8:(j * w + i) * 8 + 8]
        g, fr, fg_, fb, br, bg_, bb, ba = b
        if ba: img.paste((br, bg_, bb), (i * scale, j * scale, (i + 1) * scale, (j + 1) * scale))
        img.paste((fr, fg_, fb), (i * scale, j * scale), glyph(g))
img.save(out)
