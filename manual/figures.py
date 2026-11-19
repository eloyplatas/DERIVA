"""Figures shared by both manuals: annotated panels and the signal-flow diagram."""

MAIN_MARKERS = [  # number, x, y in panel units (1040 x 520)
    (1, 44, 186), (2, 112, 292), (3, 206, 242), (4, 294, 190), (5, 372, 92),
    (6, 670, 190), (7, 758, 242), (8, 852, 292), (9, 936, 186),
    (10, 80, 455), (11, 280, 455), (12, 452, 396), (13, 480, 455), (14, 680, 455), (15, 880, 455),
    (16, 511, 52), (17, 640, 52), (18, 24, 81),
]

BEADS_MARKERS = [  # panel units (1040 x 300)
    (1, 28, 80), (2, 56, 178), (3, 164, 178),
    (4, 282, 74), (5, 372, 74), (6, 462, 74), (7, 552, 74), (8, 292, 190),
    (9, 662, 74), (10, 752, 74), (11, 842, 74), (12, 932, 74),
    (13, 628, 213), (14, 628, 263), (15, 786, 250), (16, 952, 250),
]


def annotated(img, markers, w, h, caption):
    spans = "".join(
        f'<span class="m" style="left:{x / w * 100:.2f}%;top:{y / h * 100:.2f}%">{n}</span>' for n, x, y in markers)
    return f'<figure><div class="annot"><img src="IMG/{img}"/>{spans}</div><figcaption>{caption}</figcaption></figure>'


def main_panel(caption):
    return annotated("main.png", MAIN_MARKERS, 1040, 520, caption)


def beads_panel(caption):
    return annotated("beads.png", BEADS_MARKERS, 1040, 300, caption)


def signal_flow(t):
    """t: dict of translated labels."""
    def box(x, y, w, h, title, sub="", accent=False):
        fill = "#efeaff" if accent else "#ffffff"
        stroke = "#7a5cff" if accent else "#3a3a44"
        sw = 1.6 if accent else 1.0
        s = f'<rect x="{x}" y="{y}" width="{w}" height="{h}" rx="7" fill="{fill}" stroke="{stroke}" stroke-width="{sw}"/>'
        cy = y + h / 2 + (-3 if sub else 4)
        s += f'<text x="{x + w / 2}" y="{cy}" text-anchor="middle" font-family="Montserrat" font-weight="700" font-size="11" fill="#15151a">{title}</text>'
        if sub:
            s += f'<text x="{x + w / 2}" y="{cy + 13}" text-anchor="middle" font-family="Montserrat" font-weight="500" font-size="8.5" fill="#5b5b66">{sub}</text>'
        return s

    def arrow(d, dashed=False, color="#3a3a44"):
        da = ' stroke-dasharray="5 4"' if dashed else ""
        return f'<path d="{d}" fill="none" stroke="{color}" stroke-width="1.2"{da} marker-end="url(#ah)"/>'

    def label(x, y, txt, anchor="middle", color="#5b5b66"):
        return f'<text x="{x}" y="{y}" text-anchor="{anchor}" font-family="Montserrat" font-weight="600" font-size="8.5" fill="{color}">{txt}</text>'

    W, H = 700, 400
    s = f'<svg viewBox="0 0 {W} {H}" width="100%" xmlns="http://www.w3.org/2000/svg">'
    s += '<defs><marker id="ah" viewBox="0 0 10 10" refX="9" refY="5" markerWidth="6" markerHeight="6" orient="auto-start-reverse"><path d="M0 0L10 5L0 10z" fill="#3a3a44"/></marker></defs>'
    # row 1: input chain
    s += box(10, 20, 100, 44, t["in"], t["in_sub"])
    s += box(140, 20, 100, 44, t["gate"], t["gate_sub"])
    s += box(270, 20, 100, 44, "DRIVE", "tanh · ADAA")
    s += box(400, 20, 110, 44, "RECORD", t["rec_sub"])
    s += '<circle cx="580" cy="42" r="14" fill="#fff" stroke="#3a3a44"/><text x="580" y="46.5" text-anchor="middle" font-family="Montserrat" font-weight="700" font-size="13">+</text>'
    s += arrow("M110 42H138") + arrow("M240 42H268") + arrow("M370 42H398") + arrow("M510 42H564")
    # row 2: write path
    s += box(470, 110, 100, 44, "SUP | COM", t["comp_sub"])
    s += box(340, 110, 100, 44, t["lim"], t["lim_sub"])
    s += box(60, 100, 250, 64, t["mem"], t["mem_sub"], accent=True)
    s += arrow("M580 56V132H572") + arrow("M470 132H442") + arrow("M340 132H312")
    # row 3: feedback loop
    s += box(340, 200, 100, 44, "BLUR", t["blur_sub"])
    s += box(470, 200, 100, 44, t["filt"], "DC · HPF · LPF")
    s += box(600, 200, 90, 44, "FB", t["fb_sub"])
    s += arrow("M250 164V222H338") + arrow("M440 222H468") + arrow("M570 222H598")
    s += arrow("M645 200V42H596")
    s += label(652, 130, t["feedback"], "start")
    # row 4: output
    s += box(60, 290, 110, 44, "DRIFT", t["drift_sub"])
    s += box(210, 290, 140, 44, "BEADS", t["beads_sub"], accent=True)
    s += box(390, 290, 100, 44, "MIX", t["mix_sub"])
    s += box(530, 290, 100, 44, "OUT", "")
    s += arrow("M115 164V288") + arrow("M170 312H208") + arrow("M350 312H388") + arrow("M490 312H528")
    # dry path
    s += arrow("M60 64V80H20V372H440V336")
    s += label(250, 386, t["dry"])
    # loop return (dashed)
    s += f'<path d="M280 290V262H520V246" fill="none" stroke="#7a5cff" stroke-width="1.2" stroke-dasharray="5 4" marker-end="url(#ah)"/>'
    s += label(300, 274, t["loop"], "start", "#7a5cff")
    s += "</svg>"
    return f'<figure>{s}<figcaption>{t["caption"]}</figcaption></figure>'
