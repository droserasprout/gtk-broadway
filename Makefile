# Brotway docs (mdbook). Run from this dir.
.PHONY: lint build serve

lint:   ## markdownlint the docs sources (config: .markdownlint.jsonc)
	markdownlint 'docs/src/**/*.md'

build:  ## build the book into docs/book
	mdbook build docs

serve:  ## serve the docs at http://0.0.0.0:3000
	mdbook serve docs -n 0.0.0.0 -p 3000
