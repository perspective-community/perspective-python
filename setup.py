import os
import os.path
from setuptools import setup, find_packages
from distutils.extension import Extension
from codecs import open

here = os.path.abspath(os.path.dirname(__file__))

with open(os.path.join(here, 'README.md'), encoding='utf-8') as f:
    long_description = f.read()

sources = []
outputs = []
for path, subdirs, files in os.walk('perspective/src'):
    for name in files:
        fp = os.path.join(path, name)
        if fp.endswith('cpp'):
            outputs.append(fp.replace('.cpp', '').replace('/src', ''))
            sources.append(fp)

setup(
    name='perspective-python',
    version='0.0.11',
    description='Analytics library',
    long_description=long_description,
    url='https://github.com/timkpaine/perspective-python',
    download_url='https://github.com/timkpaine/perspective-python/archive/v0.0.11.tar.gz',
    author='Tim Paine',
    author_email='timothy.k.paine@gmail.com',
    license='Apache 2.0',
    classifiers=[
        'Development Status :: 3 - Alpha',
        'Programming Language :: Python :: 3',
        'Programming Language :: Python :: 3.3',
        'Programming Language :: Python :: 3.4',
        'Programming Language :: Python :: 3.5',
        'Programming Language :: Python :: 3.6',
        'Programming Language :: Python :: 3.7',
    ],

    keywords='analytics tools plotting',
    # packages=find_packages(exclude=['tests', ]),
    packages=find_packages(),
    include_package_data=True,
    zip_safe=False,
    ext_modules=[
        Extension(x,
                  include_dirs=["perspective/include"],
                  sources=[y],
                  libraries=["boost_python"]) for x, y in zip(outputs, sources)
    ]

)
