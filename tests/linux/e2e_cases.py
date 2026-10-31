# Cases for e2e.py. Each case: name, config (YAML), steps, optional xkb=(layout, variant) used to
# decode the output (default us), and the expected text and/or key trace ("a_" down, "a/" up).
# Steps: "d:key" down, "u:key" up, "t:key" tap, "r:key" autorepeat event, "<ms>" wait.
# Combo keys must be held together at least combo_overlap_ms (40) to trigger.

DE = "settings:\n  layout: de\n"
DF = "d:d d:f 50 u:d u:f"  # press the d + f combo

CASES = [
    # Text snippets are typed as key presses on the host layout
    dict(name="passthrough", config="combos:\n  d + f: esc\n", steps="t:h t:i", text="hi"),
    dict(name="combo", config="combos:\n  d + f: esc\n", steps=DF + " t:x",
         trace="esc_ esc/ x_ x/"),
    dict(name="snippet us", config='combos:\n  d + f: "a@[]{}~^`|\\\\ <>"\n', steps=DF,
         text="a@[]{}~^`|\\ <>"),
    dict(name="snippet de", config=DE + 'combos:\n  d + f: "Grüße @€{}[]\\\\|~^`é ê <>"\n',
         xkb=("de", ""), steps=DF, text="Grüße @€{}[]\\|~^`é ê <>"),
    dict(name="snippet fr", config='settings:\n  layout: fr\ncombos:\n  d + f: "abc qwz 123 @#{}~ éèàç ê"\n',
         xkb=("fr", ""), steps=DF, text="abc qwz 123 @#{}~ éèàç ê"),
    dict(name="snippet neo", config='settings:\n  layout: neo\ncombos:\n  d + f: "hallo ()[]{}<> äöüß"\n',
         xkb=("de", "neo"), steps=DF, text="hallo ()[]{}<> äöüß"),
    dict(name="snippet es", config='settings:\n  layout: es\ncombos:\n  d + f: "ñ []{} \\\\@# á"\n',
         xkb=("es", ""), steps=DF, text="ñ []{} \\@# á"),
    dict(name="snippet it", config='settings:\n  layout: it\ncombos:\n  d + f: "è []{} @# ò"\n',
         xkb=("it", ""), steps=DF, text="è []{} @# ò"),
    dict(name="snippet newline tab", config='combos:\n  d + f: "a\\tb\\nc"\n', steps=DF,
         trace="a_ a/ tab_ tab/ b_ b/ enter_ enter/ c_ c/"),
]

# Combo timing: overlap (held together) and gap (between presses)
TIMING = DE + 'combos:\n  d + k: "hä?1"\n'
CASES += [
    dict(name="combo held together", config=TIMING, xkb=("de", ""), steps="d:d d:k 120 u:d u:k 300",
         text="hä?1"),
    dict(name="combo keys without overlap are typed", config=TIMING, xkb=("de", ""),
         steps="d:d d:k u:d u:k 300", text="dk"),
    dict(name="combo gap too long", config=TIMING, xkb=("de", ""), steps="d:d 200 d:k 60 u:d u:k 300",
         text="dk"),
    dict(name="combo gap from settings", config=DE + '  combo_gap_ms: 300\ncombos:\n  d + k: "hä?1"\n',
         xkb=("de", ""),
         steps="d:d 200 d:k 60 u:d u:k 300", text="hä?1"),
]

FULL = """
settings:
  layout: de
combos:
  d + f: leftshift
  j + k: backspace
  s + d: "hä?"
tap_hold:
  capslock:
    tap: esc
    hold: leftctrl
  space:
    tap: space
    layer: nav
layers:
  nav:
    h: left
    l: right
    u: "{ü}"
one_shot:
  leftshift: 1500
leader:
  key: rightctrl
  timeout_ms: 1000
  sequences:
    "g s": "git status\\n"
tap_dance:
  q:
    tap: q
    double_tap: esc
    hold: leftalt
"""
X = ("de", "")
SD = "d:s d:d 50 u:s u:d"  # the s + d snippet combo
CASES += [
    dict(name="tap-hold tap", config=FULL, xkb=X, steps="t:capslock 300", trace="esc_ esc/"),
    dict(name="tap-hold hold", config=FULL, xkb=X, steps="d:capslock 300 t:c u:capslock",
         trace="leftctrl_ c_ c/ leftctrl/"),
    dict(name="layer key", config=FULL, xkb=X, steps="d:space 50 t:h 50 u:space", trace="left_ left/"),
    dict(name="layer snippet", config=FULL, xkb=X, steps="d:space 50 t:u 50 u:space", text="{ü}"),
    dict(name="layer key tap", config=FULL, xkb=X, steps="t:space 300", text=" "),
    dict(name="roll off layer key", config=FULL, xkb=X, steps="d:space 30 d:a u:space u:a", text=" a"),
    dict(name="combo held as shift", config=FULL, xkb=X, steps="d:d d:f 300 t:a t:b u:d u:f", text="AB"),
    dict(name="combo snippet", config=FULL, xkb=X, steps=SD, text="hä?"),
    dict(name="one-shot shift", config=FULL, xkb=X, steps="t:leftshift 50 t:a t:b", text="Ab"),
    dict(name="leader", config=FULL, xkb=X, steps="t:rightctrl t:g t:s", text="git status\r"),
    dict(name="tap dance double", config=FULL, xkb=X, steps="t:q 40 t:q 400", trace="esc_ esc/"),
    dict(name="tap dance single", config=FULL, xkb=X, steps="t:q 400", text="q"),
    dict(name="tap dance hold", config=FULL, xkb=X, steps="d:q 400 t:x u:q", trace="leftalt_ x_ x/ leftalt/"),
    dict(name="fast typing", config=FULL, xkb=X, steps="t:h t:a t:l t:l t:o t:space t:w t:e t:l t:t",
         text="hallo welt"),
    # A snippet comes out as written while Shift or AltGr is held; the held key works again after it
    dict(name="snippet while shift held", config=FULL, xkb=X,
         steps="d:leftshift 300 " + SD + " t:a u:leftshift", text="hä?A"),
    dict(name="snippet while altgr held", config=FULL, xkb=X,
         steps="d:rightalt 300 " + SD + " t:e u:rightalt", text="hä?€"),
    dict(name="snippet uses up one-shot shift", config=FULL, xkb=X,
         steps="t:leftshift 50 " + SD + " t:a", text="hä?a"),
    dict(name="combo shift and layer snippet", config=FULL, xkb=X,
         steps="d:d d:f 300 d:space 50 t:u 50 u:space t:a u:d u:f", text="{ü}A"),
    dict(name="neo snippet while mod3 held", config='settings:\n  layout: neo\ncombos:\n  d + f: "hallo ()"\n',
         xkb=("de", "neo"), steps="d:capslock 300 " + DF + " u:capslock", text="hallo ()"),
    dict(name="us snippet while right alt held", config='combos:\n  d + f: "ab"\n',
         steps="d:rightalt 300 " + DF + " u:rightalt", text="ab"),
    # Autorepeat of a held key comes from the output device, not doubled
    dict(name="held key with source autorepeat", config="combos:\n  d + f: esc\n",
         steps="d:a 250 " + "r:a " * 20 + "u:a", text="a"),
    dict(name="held key", config="combos:\n  d + f: esc\n", steps="d:a 1000 u:a", text="a"),
]
