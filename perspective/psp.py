def psp(data):
    from IPython.display import display
    bundle = {}
    bundle['application/psp'] = data
    display(bundle, raw=True)
