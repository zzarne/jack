#!/usr/bin/env python3

"""Setup script for the jack module distribution."""

from setuptools import setup, find_packages
import jack.version

PACKAGES = find_packages(exclude=find_packages(where="deprecated"))

REQUIRES = [
    'eyeD3',
]

setup(
    name=jack.version.name,
    version=jack.version.version,
    description="A frontend for several cd-rippers and mp3 encoders",
    author=jack.version.author,
    author_email=jack.version.email,
    url=jack.version.url,
    license=jack.version.license,
    # platform     = "POSIX",

    python_requires='>3',
    install_requires=REQUIRES,
    packages=PACKAGES,

    # include_package_data=True,
    # zip_safe=False,
    # test_suite="tests",
    entry_points={"console_scripts": ["jack = jack.__main__:main"]},
)
