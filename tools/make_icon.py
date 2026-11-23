# -*- coding: utf-8 -*-
"""从一个 65x65 的 PNG 图标重新绘制成多尺寸 .ico（矢量式重绘，避免放大模糊）"""
from PIL import Image, ImageDraw

SRC = r"E:\MCL\res\icon_src.png"
OUT_ICO = r"E:\MCL\res\mcl.ico"
OUT_PNG = r"E:\MCL\res\icon_256.png"

ACCENT = (60, 133, 39, 255)    # 背景 #3C8527
LIGHT = (122, 190, 92, 255)    # 草方块亮面
DARK = (88, 152, 66, 255)      # 草方块暗面

# 以 256 为基准的几何比例（源自 65x65 原图测量）
def render(size: int) -> Image.Image:
    ss = 8                       # 超采样倍数
    S = size * ss
    im = Image.new("RGBA", (S, S), (0, 0, 0, 0))
    d = ImageDraw.Draw(im)
    r = int(S * 0.22)            # 圆角半径
    d.rounded_rectangle([0, 0, S - 1, S - 1], radius=r, fill=ACCENT)
    x0, x1 = int(S * 0.2154), int(S * 0.8308)
    y0, y1 = int(S * 0.1846), int(S * 0.4308)
    d.rectangle([x0, y0, x1, y1], fill=LIGHT)
    y2, y3 = int(S * 0.4462), int(S * 0.8000)
    d.rectangle([x0, y2, x1, y3], fill=DARK)
    return im.resize((size, size), Image.LANCZOS)


master = render(256)
master.save(OUT_PNG)
sizes = [(16, 16), (24, 24), (32, 32), (48, 48), (64, 64), (128, 128), (256, 256)]
master.save(OUT_ICO, format="ICO", sizes=sizes)
print("wrote", OUT_ICO)
