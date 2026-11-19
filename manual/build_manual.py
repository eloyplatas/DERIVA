#!/usr/bin/env python3
"""Builds the Deriva user manuals (English and Spanish) as PDF with WeasyPrint."""
import pathlib
import sys

from weasyprint import HTML

ROOT = pathlib.Path(__file__).resolve().parent
FONTS = ROOT.parent / "Resources" / "fonts"
IMG = ROOT / "img"
VERSION = "1.1.1"

sys.path.insert(0, str(ROOT))
from content_en import CONTENT as EN  # noqa: E402
from content_es import CONTENT as ES  # noqa: E402

CSS = """
@font-face { font-family: 'Montserrat'; font-weight: 400; src: url('FONTS/Montserrat-Regular.ttf'); }
@font-face { font-family: 'Montserrat'; font-weight: 500; src: url('FONTS/Montserrat-Medium.ttf'); }
@font-face { font-family: 'Montserrat'; font-weight: 600; src: url('FONTS/Montserrat-SemiBold.ttf'); }
@font-face { font-family: 'Montserrat'; font-weight: 700; src: url('FONTS/Montserrat-Bold.ttf'); }

@page {
  size: A4; margin: 20mm 18mm 20mm 18mm;
  @top-left { content: "DERIVA"; font: 600 7pt Montserrat; letter-spacing: 2.5pt; color: #8a8a92; }
  @top-right { content: string(chapter); font: 500 7pt Montserrat; letter-spacing: 1.2pt; color: #8a8a92; text-transform: uppercase; }
  @bottom-center { content: counter(page); font: 600 7.5pt Montserrat; color: #7a5cff; }
}
@page cover { margin: 0; background: #0f0f12; @top-left { content: none; } @top-right { content: none; } @bottom-center { content: none; } }
@page :first { margin: 0; }

html { font-family: Montserrat; font-size: 9.2pt; line-height: 1.55; color: #1c1c22; }
body { margin: 0; }
h1 { string-set: chapter content(); font-weight: 700; font-size: 21pt; letter-spacing: 1.2pt; color: #15151a;
     margin: 0 0 4mm 0; padding-top: 2mm; page-break-before: always; }
h1 .num { color: #7a5cff; margin-right: 3mm; }
h2 { font-weight: 700; font-size: 12.5pt; color: #3d2fa0; margin: 7mm 0 2mm 0; letter-spacing: .4pt; }
h3 { font-weight: 600; font-size: 10pt; color: #15151a; margin: 5mm 0 1.5mm 0; }
p { margin: 0 0 2.6mm 0; text-align: left; }
.lead { font-size: 10.6pt; line-height: 1.6; color: #2b2b33; }
ul, ol { margin: 0 0 3mm 0; padding-left: 5mm; }
li { margin-bottom: 1.2mm; }
strong { font-weight: 600; }
em { font-style: normal; color: #3d2fa0; font-weight: 500; }
code { font-family: 'DejaVu Sans Mono', monospace; font-size: 8pt; background: #f1effa; padding: .3mm 1.2mm; border-radius: 1mm; }
table { width: 100%; border-collapse: collapse; margin: 2mm 0 4mm 0; font-size: 8.4pt; page-break-inside: auto; }
th { text-align: left; font-weight: 600; font-size: 7.6pt; letter-spacing: .8pt; text-transform: uppercase; color: #fff;
     background: #24242c; padding: 1.8mm 2.2mm; }
td { padding: 1.6mm 2.2mm; vertical-align: top; border-bottom: .3mm solid #e4e2ee; }
tr:nth-child(even) td { background: #f8f7fc; }
td:first-child { font-weight: 600; white-space: nowrap; }
.tip, .note { border-left: 1.2mm solid #7a5cff; background: #f3f0ff; padding: 2.6mm 3.5mm; margin: 3mm 0 4mm 0; border-radius: 0 1.5mm 1.5mm 0; }
.note { border-left-color: #c9a227; background: #fbf6e6; }
.tip b, .note b { display: block; font-weight: 700; font-size: 7.4pt; letter-spacing: 1.4pt; text-transform: uppercase; margin-bottom: .8mm; color: #3d2fa0; }
.note b { color: #8a6d00; }
figure { margin: 3mm 0 5mm 0; page-break-inside: avoid; }
figure img { width: 100%; border-radius: 2mm; display: block; }
figcaption { font-size: 7.6pt; color: #6c6c76; margin-top: 1.6mm; }
.annot { position: relative; }
.annot .m { position: absolute; width: 5.2mm; height: 5.2mm; margin-left: -2.6mm; margin-top: -2.6mm; border-radius: 50%;
            background: #7a5cff; color: #fff; font-weight: 700; font-size: 7pt; line-height: 5.2mm; text-align: center;
            border: .45mm solid #fff; box-shadow: 0 0 1mm rgba(0,0,0,.5); }
.two { display: flex; gap: 5mm; }
.two > div { flex: 1; }
.timeline { border-left: .6mm solid #d8d2fb; margin: 2mm 0 4mm 2mm; padding-left: 5mm; }
.timeline .ev { margin-bottom: 3mm; position: relative; }
.timeline .ev:before { content: ""; position: absolute; left: -6.6mm; top: 1.2mm; width: 2.6mm; height: 2.6mm; border-radius: 50%; background: #7a5cff; }
.timeline .yr { font-weight: 700; color: #3d2fa0; margin-right: 2mm; }
.toc { margin-top: 2mm; }
.toc a { color: #1c1c22; text-decoration: none; }
.toc li { list-style: none; margin: 0 0 1.6mm 0; font-weight: 500; }
.toc li a::after { content: leader('.') target-counter(attr(href), page); color: #7a5cff; font-weight: 600; }
.toc ol { padding-left: 0; }
.recipe { border: .3mm solid #e1ddf3; border-radius: 2mm; padding: 3mm 4mm; margin: 0 0 4mm 0; page-break-inside: avoid; }
.recipe h3 { margin-top: 0; color: #3d2fa0; }
.recipe .set { font-size: 8pt; color: #55555f; }
.small { font-size: 7.8pt; color: #55555f; }

/* cover */
.cover { page: cover; width: 210mm; height: 297mm; position: relative; color: #eeeeef; overflow: hidden; }
.cover .glow { position: absolute; left: 25mm; top: 70mm; width: 160mm; height: 160mm; border-radius: 50%;
               background: radial-gradient(circle, rgba(122,92,255,.33) 0%, rgba(122,92,255,0) 65%); }
.cover .brand { position: absolute; left: 20mm; top: 24mm; font-weight: 700; font-size: 44pt; letter-spacing: 9pt; }
.cover .sub { position: absolute; left: 21mm; top: 46mm; font-weight: 600; font-size: 9pt; letter-spacing: 4pt; color: #8a8a92; }
.cover .hero { position: absolute; left: 14mm; top: 78mm; width: 182mm; border-radius: 3mm; box-shadow: 0 4mm 12mm rgba(0,0,0,.6); }
.cover .orbits { position: absolute; left: 0; top: 0; }
.cover .title { position: absolute; left: 20mm; bottom: 40mm; font-weight: 600; font-size: 15pt; letter-spacing: 3pt; }
.cover .meta { position: absolute; left: 20mm; bottom: 30mm; font-weight: 500; font-size: 8.5pt; letter-spacing: 2pt; color: #8a8a92; }
.cover .line { position: absolute; left: 20mm; right: 20mm; bottom: 52mm; height: .3mm; background: #7a5cff; }
"""


def cover(c):
    return f"""
<section class="cover">
  <div class="glow"></div>
  <svg class="orbits" width="210mm" height="297mm" viewBox="0 0 210 297">
    <g fill="none" stroke="#ffffff" stroke-opacity="0.06" stroke-width="0.3">
      <circle cx="170" cy="40" r="22"/><circle cx="170" cy="40" r="30"/><circle cx="170" cy="40" r="38"/><circle cx="170" cy="40" r="46"/>
    </g>
    <circle cx="190.5" cy="28" r="1.3" fill="#b4a3ff"/><circle cx="141" cy="57" r="1.1" fill="#b4a3ff"/>
    <circle cx="176" cy="85.5" r="1.2" fill="#b4a3ff"/><circle cx="208" cy="58" r="1" fill="#b4a3ff"/>
  </svg>
  <div class="brand">DERIVA</div>
  <div class="sub">PRIME MEMORY DRIFTER</div>
  <img class="hero" src="{IMG / 'full.png'}"/>
  <div class="line"></div>
  <div class="title">{c['cover_title']}</div>
  <div class="meta">{c['cover_meta'].format(version=VERSION)}</div>
</section>
"""


def toc(c):
    items = "".join(f'<li><a href="#{sid}">{title}</a></li>' for sid, title in c["toc"])
    return f'<h1 id="toc" style="page-break-before: always">{c["toc_title"]}</h1><div class="toc"><ol>{items}</ol></div>'


def build(c, out):
    html = f"""<!DOCTYPE html><html lang="{c['lang']}"><head><meta charset="utf-8"><title>Deriva — {c['cover_title']}</title>
<style>{CSS.replace('FONTS', FONTS.as_uri())}</style></head><body>
{cover(c)}
{toc(c)}
{c['body'].replace('IMG/', IMG.as_uri() + '/')}
</body></html>"""
    HTML(string=html, base_url=str(ROOT)).write_pdf(out)
    print("wrote", out)


if __name__ == "__main__":
    outdir = ROOT / "out"
    outdir.mkdir(exist_ok=True)
    build(EN, outdir / f"Deriva-{VERSION}-Manual-EN.pdf")
    build(ES, outdir / f"Deriva-{VERSION}-Manual-ES.pdf")
