# Shared look and machinery for the printable reference sheets in this folder.
#
#   quickref.py  ->  QUICKREF.pdf   modes, first-time setup, global settings
#   features.py  ->  FEATURES.pdf   what differs from stock, and everything the firmware does
#
# Needs reportlab and svglib:  python -m pip install reportlab svglib
# Then:                        python quickref.py QUICKREF.pdf
#
# The keypad icons in icons/ are the panel's own artwork, taken from the project README where
# each one is already paired with the function it performs - which is more reliable than reading
# them off the keypad overlay and guessing. They are SVG, so they stay sharp at any print size.

import os

from reportlab.lib.pagesizes import A4
from reportlab.lib import colors
from reportlab.lib.units import mm
from reportlab.lib.styles import ParagraphStyle
from reportlab.lib.enums import TA_LEFT
from reportlab.platypus import (BaseDocTemplate, PageTemplate, Frame, Paragraph,
                                Spacer, Table, TableStyle, PageBreak)
from reportlab.graphics.shapes import Drawing, Group
from svglib.svglib import svg2rlg

W, H = A4
MARGIN = 13 * mm

INK = colors.HexColor("#16202b")
DIM = colors.HexColor("#5b6b7a")
LINE = colors.HexColor("#c9d4de")
ACCENT = colors.HexColor("#0b6ec9")
WARM = colors.HexColor("#b4530a")
SOFT = colors.HexColor("#eef3f8")
SOFT2 = colors.HexColor("#f7f9fb")
GOOD = colors.HexColor("#1c7c40")
LCDBG = colors.HexColor("#1d3f6e")


def style(name, size, leading=None, colour=INK, bold=False, space=0):
    return ParagraphStyle(
        name, fontName="Helvetica-Bold" if bold else "Helvetica",
        fontSize=size, leading=leading or size + 1.6, textColor=colour,
        spaceAfter=space, alignment=TA_LEFT)


S_H2 = style("h2", 10.5, 12, ACCENT, bold=True, space=3)
S_BODY = style("body", 7.4, 9)
S_DIM = style("dim", 7.0, 8.6, DIM)
S_CELL = style("cell", 7.0, 8.4)
S_CELL_B = style("cellb", 7.0, 8.4, bold=True)
S_KEY = style("key", 7.0, 8.4, colors.white, bold=True)
S_NOTE = style("note", 7.0, 8.8, WARM)
S_LCD = ParagraphStyle("lcd", fontName="Courier-Bold", fontSize=8.4, leading=10.4,
                       textColor=colors.HexColor("#eaf2ff"), alignment=TA_LEFT)


def P(t, s=S_CELL):
    return Paragraph(t, s)


# ---------------------------------------------------------------------------
# Keypad icons
# ---------------------------------------------------------------------------

ICON_DIR = os.path.join(os.path.dirname(os.path.abspath(__file__)), "icons")
_icon_cache = {}


def icon(name, size=11):
    """A keypad icon scaled to `size` points, as a flowable."""
    key = (name, size)
    if key in _icon_cache:
        return _icon_cache[key]
    src = svg2rlg(os.path.join(ICON_DIR, name + ".svg"))
    scale = size / float(src.width)
    d = Drawing(size, size)
    g = Group(src)
    g.scale(scale, scale)
    d.add(g)
    _icon_cache[key] = d
    return d


def _bare(t):
    t.setStyle(TableStyle([
        ("VALIGN", (0, 0), (-1, -1), "MIDDLE"),
        ("ALIGN", (0, 0), (-1, -1), "CENTRE"),
        ("LEFTPADDING", (0, 0), (-1, -1), 0),
        ("RIGHTPADDING", (0, 0), (-1, -1), 0),
        ("TOPPADDING", (0, 0), (-1, -1), 0),
        ("BOTTOMPADDING", (0, 0), (-1, -1), 0),
    ]))
    return t


def icon_row(names, size=11, gap=1.6, joiner=""):
    """Several keys side by side. The joiner is empty because they are usually alternatives."""
    cells, widths = [], []
    for i, n in enumerate(names):
        if i and joiner:
            cells.append(P(joiner, S_CELL))
            widths.append(5)
        cells.append(icon(n, size))
        widths.append(size + gap)
    return _bare(Table([cells], colWidths=widths, rowHeights=[size + 2]))


def icon_key(name, twice=False, size=11):
    """One key, optionally marked as pressed twice to reach its hidden sibling."""
    cells, widths = [icon(name, size)], [size + 2]
    if twice:
        cells.append(P("&#215;2", S_CELL))
        widths.append(12)
    t = Table([cells], colWidths=widths, rowHeights=[size + 2])
    t.setStyle(TableStyle([
        ("VALIGN", (0, 0), (-1, -1), "MIDDLE"),
        ("LEFTPADDING", (0, 0), (-1, -1), 0),
        ("RIGHTPADDING", (0, 0), (-1, -1), 1),
        ("TOPPADDING", (0, 0), (-1, -1), 0),
        ("BOTTOMPADDING", (0, 0), (-1, -1), 0),
    ]))
    return t


# ---------------------------------------------------------------------------
# Shared blocks
# ---------------------------------------------------------------------------

def zebra(data, colWidths, head=True, box=True):
    """A table with a dark header row and alternating row tint."""
    t = Table(data, colWidths=colWidths, repeatRows=1 if head else 0)
    st = [
        ("VALIGN", (0, 0), (-1, -1), "TOP"),
        ("LEFTPADDING", (0, 0), (-1, -1), 3.5),
        ("RIGHTPADDING", (0, 0), (-1, -1), 3.5),
        ("TOPPADDING", (0, 0), (-1, -1), 2.8),
        ("BOTTOMPADDING", (0, 0), (-1, -1), 2.8),
        ("LINEBELOW", (0, 0), (-1, -1), 0.4, LINE),
    ]
    if head:
        st.append(("BACKGROUND", (0, 0), (-1, 0), INK))
    if box:
        st.append(("BOX", (0, 0), (-1, -1), 0.6, LINE))
    for i in range(1, len(data)):
        if i % 2 == 0:
            st.append(("BACKGROUND", (0, i), (-1, i), SOFT2))
    t.setStyle(TableStyle(st))
    return t


def section_block(title, rows, width, colour=ACCENT):
    """A titled two-column block: name on the left, what it means on the right."""
    data = [[P(title, S_KEY), ""]]
    for name, desc in rows:
        data.append([P(name, S_CELL_B), P(desc, S_CELL)])
    t = Table(data, colWidths=[width * 0.34, width * 0.66])
    st = [
        ("BACKGROUND", (0, 0), (-1, 0), colour),
        ("SPAN", (0, 0), (-1, 0)),
        ("VALIGN", (0, 0), (-1, -1), "TOP"),
        ("LEFTPADDING", (0, 0), (-1, -1), 3.5),
        ("RIGHTPADDING", (0, 0), (-1, -1), 3.5),
        ("TOPPADDING", (0, 0), (-1, -1), 2.2),
        ("BOTTOMPADDING", (0, 0), (-1, -1), 2.2),
        ("BOX", (0, 0), (-1, -1), 0.6, LINE),
        ("LINEBELOW", (0, 1), (-1, -2), 0.35, LINE),
    ]
    for i in range(1, len(data)):
        if i % 2 == 1:
            st.append(("BACKGROUND", (0, i), (-1, i), SOFT2))
    t.setStyle(TableStyle(st))
    return t


def two_columns(left, right, gap=5 * mm):
    """Two independent stacks side by side, each a list of flowables."""
    colw = (W - 2 * MARGIN - gap) / 2.0
    t = Table([[left, right]], colWidths=[colw, colw])
    t.setStyle(TableStyle([
        ("VALIGN", (0, 0), (-1, -1), "TOP"),
        ("LEFTPADDING", (0, 0), (-1, -1), 0),
        ("RIGHTPADDING", (0, 0), (0, 0), gap),
        ("RIGHTPADDING", (1, 0), (1, 0), 0),
        ("TOPPADDING", (0, 0), (-1, -1), 0),
        ("BOTTOMPADDING", (0, 0), (-1, -1), 0),
    ]))
    return t


def column_width(gap=5 * mm):
    return (W - 2 * MARGIN - gap) / 2.0


def stack(blocks, space=4):
    """Interleaves spacers between blocks so a column reads as separate cards."""
    out = []
    for i, b in enumerate(blocks):
        out.append(b)
        if i != len(blocks) - 1:
            out.append(Spacer(1, space))
    return out


def numbered(rows, numw=7 * mm, iconw=None):
    """A numbered procedure. Each row is (number, text) or (number, icons, text)."""
    widths = [numw] + ([iconw] if iconw else []) + [None]
    t = Table(rows, colWidths=widths)
    t.setStyle(TableStyle([
        ("VALIGN", (0, 0), (-1, -1), "TOP"),
        ("LEFTPADDING", (0, 0), (-1, -1), 3.5),
        ("TOPPADDING", (0, 0), (-1, -1), 2.4),
        ("BOTTOMPADDING", (0, 0), (-1, -1), 2.4),
        ("BACKGROUND", (0, 0), (-1, -1), SOFT),
        ("BOX", (0, 0), (-1, -1), 0.6, LINE),
        ("LINEBELOW", (0, 0), (-1, -2), 0.4, colors.white),
    ]))
    return t


# ---------------------------------------------------------------------------
# Page furniture and build
# ---------------------------------------------------------------------------

def _header(title, subtitles, footer_left):
    def draw(canvas, doc):
        canvas.saveState()
        band_h = 15 * mm
        canvas.setFillColor(INK)
        canvas.rect(0, H - band_h, W, band_h, stroke=0, fill=1)
        canvas.setFillColor(colors.white)
        canvas.setFont("Helvetica-Bold", 13)
        canvas.drawString(MARGIN, H - 9.6 * mm, title)
        canvas.setFont("Helvetica", 8.2)
        canvas.setFillColor(colors.HexColor("#9fb3c4"))
        i = min(doc.page, len(subtitles)) - 1
        canvas.drawRightString(W - MARGIN, H - 9.6 * mm,
                               "%d / %d   %s" % (doc.page, len(subtitles), subtitles[i]))

        canvas.setFillColor(DIM)
        canvas.setFont("Helvetica", 6.4)
        canvas.drawString(MARGIN, 7.5 * mm, footer_left)
        canvas.drawRightString(W - MARGIN, 7.5 * mm,
                               "github.com/timm052/nanoels  ·  branch h4-plus")
        canvas.setStrokeColor(LINE)
        canvas.setLineWidth(0.5)
        canvas.line(MARGIN, 9.6 * mm, W - MARGIN, 9.6 * mm)
        canvas.restoreState()
    return draw


def build(path, title, pages, subtitles, footer_left=""):
    doc = BaseDocTemplate(path, pagesize=A4,
                          leftMargin=MARGIN, rightMargin=MARGIN,
                          topMargin=19 * mm, bottomMargin=12 * mm,
                          title=title, author="nanoels h4-plus")
    frame = Frame(MARGIN, 12 * mm, W - 2 * MARGIN, H - 19 * mm - 12 * mm, id="f",
                  leftPadding=0, rightPadding=0, topPadding=0, bottomPadding=0)
    doc.addPageTemplates([PageTemplate(id="p", frames=[frame],
                                       onPage=_header(title, subtitles, footer_left))])
    story = []
    for i, page in enumerate(pages):
        if i:
            story.append(PageBreak())
        story.extend(page)
    doc.build(story)
    print("wrote", path)
