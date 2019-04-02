build: ## build the package
	python3 setup.py build

tests: ## Clean and Make unit tests
	python3 -m pytest -v perspective --cov=perspective

test: clean build lint ## run the tests for travis CI
	@ python3 -m pytest -v perspective --cov=perspective

lint: ## run linter
	flake8 perspective 

annotate: ## MyPy type annotation check
	mypy -s perspective

annotate_l: ## MyPy type annotation check - count only
	mypy -s perspective | wc -l 

clean: ## clean the repository
	find . -name "__pycache__" | xargs  rm -rf 
	find . -name "*.pyc" | xargs rm -rf 
	find . -name ".ipynb_checkpoints" | xargs  rm -rf 
	rm -rf .coverage cover htmlcov logs build dist *.egg-info
	make -C ./docs clean
	find . -name "*.so"  | xargs rm -rf
	find . -name "*.a"  | xargs rm -rf

labextension: install ## enable labextension
	jupyter labextension install @jpmorganchase/perspective-jupyterlab

install:  ## install to site-packages
	python3 -m pip install .

preinstall:  ## install dependencies
	python3 -m pip install -r requirements.txt

docs:  ## make documentation
	make -C ./docs html

micro:  ## steps before dist, defaults to previous tag + one micro
	. scripts/deploy.sh MICRO

minor:  ## steps before dist, defaults to previous tag + one micro
	. scripts/deploy.sh MINOR

major:  ## steps before dist, defaults to previous tag + one micro
	. scripts/deploy.sh MAJOR

dist:  ## dist to pypi
	python3 setup.py sdist upload -r pypi

# Thanks to Francoise at marmelab.com for this
.DEFAULT_GOAL := help
help:
	@grep -E '^[a-zA-Z_-]+:.*?## .*$$' $(MAKEFILE_LIST) | sort | awk 'BEGIN {FS = ":.*?## "}; {printf "\033[36m%-30s\033[0m %s\n", $$1, $$2}'

print-%:
	@echo '$*=$($*)'

.PHONY: clean test tests help annotate annotate_l docs dist build buildext buildjs buildip buildd builddebug buildipd
