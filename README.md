# perspective-python
Python APIs for [perspective](https://github.com/finos/perspective) front end

# This package now lives partially under [Perspective](https://github.com/finos/perspective)

[![Build Status](https://travis-ci.org/timkpaine/perspective-python.svg?branch=master)](https://travis-ci.org/timkpaine/perspective-python)
[![GitHub issues](https://img.shields.io/github/issues/timkpaine/perspective-python.svg)]()
[![codecov](https://codecov.io/gh/timkpaine/perspective-python/branch/master/graph/badge.svg)](https://codecov.io/gh/timkpaine/perspective-python)
[![BCH compliance](https://bettercodehub.com/edge/badge/timkpaine/perspective-python?branch=master)](https://bettercodehub.com/)
[![PyPI](https://img.shields.io/pypi/v/perspective-python.svg)](https://pypi.python.org/pypi/perspective-python)
[![PyPI](https://img.shields.io/pypi/l/perspective-python.svg)](https://pypi.python.org/pypi/perspective-python)
[![Docs](https://img.shields.io/readthedocs/perspective-python.svg)](https://perspective-python.readthedocs.io)
[![Gitter](https://img.shields.io/gitter/room/nwjs/nw.js.svg)](https://gitter.im/finos/perspective)


## Install
To install the base package from pip:

`pip install perspective-python`

To Install from source:

`make install`


To install the JupyterLab extension:

`jupyter labextension install @finos/perspective-jupyterlab`

or from source:

`make labextension`

## Getting Started
[Read the docs!](http://perspective-python.readthedocs.io/en/latest/index.html)

[Example Notebooks](https://github.com/timkpaine/perspective-python/tree/master/examples)

![](https://github.com/timkpaine/perspective-python/raw/master/docs/img/scatter.png)


## Pandas Pivot integration

#### Index - Multiindex pivot
![](https://github.com/timkpaine/perspective-python/raw/master/docs/img/pandas1.png)

#### Column - Multiindex pivot
![](https://github.com/timkpaine/perspective-python/raw/master/docs/img/pandas2.png)

## C++ Integration
This package is primarily focused on integrating with the WebAssembly version of Perspective. To build the C++ side, install `perspective-python[table]`, from the [Perspective main library](https://github.com/finos/perspective/tree/master/python).

## Webserver Integration
`perspective-pyton` can be integrated with a webserver, giving you the ability to configure `perspective-viewers` in javascript from python. Right now this functionality is limited to `tornado` webservers and the `perspective-phosphor` frontend. It relies on the [phosphor-perspective-utils](https://github.com/timkpaine/phosphor-perspective-utils) javascript package.

```python3
import tornado.web
from perspective import PerspectiveHTTPMixin


class MyHandler(PerspectiveHTTPMixin, tornado.web.RequestHandler):
    def get(self):
        super(MyHandler, self).loadData(data=<data>, transfer_as_arrow=True)
        self.write(super(MyHandler, self).getData())
```


