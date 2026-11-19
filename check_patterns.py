import os
code_dir = 'code'
for f in sorted(os.listdir(code_dir)):
    if f.endswith('.h'):
        path = os.path.join(code_dir, f)
        with open(path) as fh:
            text = fh.read()
        has_reg = 'RWS_REGISTER_CLASS' in text
        has_makenew = 'RWS_MAKENEWCLASS' in text
        has_declare = 'RWS_DECLARE_CLASS' in text
        print(f + ': REG={has_reg} MAKE={has_makenew} DECLARE={has_declare}')