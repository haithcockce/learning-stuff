# Contributing to Insights Core Rules

- Dev environment setup

   1. Make sure appropriate packages are installed: python3, pip, python virtualenv, gcc

    dnf install python3 pip3 


# Insights Arch Overview

- **Context** The collection of data sources. For example, the default context
  is the current live system, but the context could be a sosreport directory,
  a sosreport tarball, insights archive, etc
- **Data Sources** How the data is collected. For example, `/etc/hosts` is a
  file while `fdisk -l` is command output. 

- Sources: 

   - [For insights quickstart dev setup](https://insights-core.readthedocs.io/en/latest/quickstart_insights_core.html)
   - [For python virtual environments](https://docs.python-guide.org/dev/virtualenvs/)
