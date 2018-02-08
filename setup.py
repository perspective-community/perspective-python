from setuptools import setup, find_packages
from codecs import open
from os import path

here = path.abspath(path.dirname(__file__))

with open(path.join(here, 'README.md'), encoding='utf-8') as f:
    long_description = f.read()

setup(
    name='perspective-python',
    version='0.0.3',
    description='Analytics library',
    long_description=long_description,
    url='https://github.com/timkpaine/perspective-python',
    download_url='https://github.com/timkpaine/perspective-python/archive/v0.0.3.tar.gz',
    author='Tim Paine',
    author_email='timothy.k.paine@gmail.com',
    license='Apache 2.0',
    classifiers=[
        'Development Status :: 3 - Alpha',
        'Programming Language :: Python :: 3',
        'Programming Language :: Python :: 3.3',
        'Programming Language :: Python :: 3.4',
        'Programming Language :: Python :: 3.5',
    ],

    keywords='analytics tools plotting',
    packages=find_packages(exclude=['tests', ]),
    include_package_data=True,
    zip_safe=False,
)
