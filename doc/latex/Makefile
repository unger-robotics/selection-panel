# =============================================================================
# LaTeX Build (XeLaTeX via latexmk) fuer latex/
#
# Struktur (aktuell):
#   latex/
#     Makefile
#     class/ content/ covers/ images/ main/ build/
#
# Ziele:
# - Minimal: wenige Targets, eine Compile-Regel
# - Robust: TEXINPUTS reproduzierbar, Default-TeX-Pfade bleiben aktiv
# - Inkrementell: Rebuild bei Änderungen in main/, content/, class/
# =============================================================================

SHELL := /bin/bash
.SHELLFLAGS := -euo pipefail -c
.DELETE_ON_ERROR:
.SUFFIXES:
.DEFAULT_GOAL := all

# --- Pfade -------------------------------------------------------------------
LATEX_DIR := $(CURDIR)
BUILD_DIR := $(LATEX_DIR)/build

# Projekt-Root ist zwei Ebenen ueber doc/latex/
ROOT    := $(abspath $(LATEX_DIR)/../..)
PDF_DST := $(ROOT)/assets/pdf

# --- LaTeX -------------------------------------------------------------------
# WICHTIG: trailing ":" -> Default-TeX-Suchpfade bleiben erhalten (article.cls etc.)
export TEXINPUTS := \
  $(LATEX_DIR)/class:\
  $(LATEX_DIR)/content:\
  $(LATEX_DIR)/covers:\
  $(LATEX_DIR)/main:\
  $(LATEX_DIR)/images:\
  $(TEXINPUTS):

LATEXMK := latexmk -pdfxe -interaction=nonstopmode -halt-on-error -file-line-error -outdir="$(BUILD_DIR)"

# --- Inputs (Templates nicht bauen) ------------------------------------------
MAIN_TEX  := $(filter-out main/main_template.tex,$(wildcard main/*.tex))
COVER_TEX := $(filter-out covers/cover-template.tex,$(wildcard covers/*.tex))

TEX_NAMES := $(notdir $(MAIN_TEX) $(COVER_TEX))
PDFS      := $(addprefix $(BUILD_DIR)/,$(TEX_NAMES:.tex=.pdf))

# --- Dependencies ------------------------------------------------------------
# Gemeinsame Dateien: Änderung triggert Rebuild aller PDFs
CONTENT_TEX := $(wildcard content/*.tex)
CLASS_FILES := $(wildcard class/*.cls class/*.sty)
IMAGE_FILES := $(wildcard images/*.pdf images/*.png images/*.jpg covers/*.pdf)
COMMON_DEPS := $(CONTENT_TEX) $(CLASS_FILES) $(IMAGE_FILES)

.PHONY: all publish clean distclean help debug

all: $(PDFS)

$(BUILD_DIR):
	@mkdir -p "$@"

# --- Compile-Regeln mit expliziten Dependencies ------------------------------
# Main-Dokumente: main/*.tex -> build/*.pdf
$(BUILD_DIR)/%.pdf: main/%.tex $(COMMON_DEPS) | $(BUILD_DIR)
	$(LATEXMK) "$<"

# Cover-Dokumente: covers/*.tex -> build/*.pdf
# Separate Regel, da Cover-PDFs keine content/-Abhängigkeit brauchen
$(BUILD_DIR)/cover-%.pdf: covers/cover-%.tex $(CLASS_FILES) | $(BUILD_DIR)
	$(LATEXMK) "$<"

publish: all
	@mkdir -p "$(PDF_DST)"
	@# Covers als Artefakte auch ins lokale images/ spiegeln (falls covers neu gebaut wurden)
	@for f in "$(BUILD_DIR)"/cover-*.pdf; do \
		[ -e "$$f" ] || continue; \
		cp -f "$$f" "$(LATEX_DIR)/images/"; \
	done
	@# Alle PDFs (inkl. Covers) nach assets/pdf
	@for f in "$(BUILD_DIR)"/*.pdf; do \
		[ -e "$$f" ] || continue; \
		cp -f "$$f" "$(PDF_DST)/"; \
	done

clean:
	@rm -f "$(BUILD_DIR)"/*.{aux,log,fls,fdb_latexmk,synctex.gz,out,toc,xdv,bbl,blg,idx,ilg,ind,lof,lot,nav,snm,vrb,lol} 2>/dev/null || true

distclean:
	@rm -rf "$(BUILD_DIR)"

# Debug: Zeigt erkannte Dateien
debug:
	@echo "=== Main-Dokumente ==="
	@echo "$(MAIN_TEX)" | tr ' ' '\n'
	@echo ""
	@echo "=== Cover-Dokumente ==="
	@echo "$(COVER_TEX)" | tr ' ' '\n'
	@echo ""
	@echo "=== Content (Kapitel) ==="
	@echo "$(CONTENT_TEX)" | tr ' ' '\n'
	@echo ""
	@echo "=== Class-Dateien ==="
	@echo "$(CLASS_FILES)" | tr ' ' '\n'
	@echo ""
	@echo "=== Ziel-PDFs ==="
	@echo "$(PDFS)" | tr ' ' '\n'

help:
	@echo "Targets:"
	@echo "  make           - build all (main + covers)"
	@echo "  make publish   - copy PDFs to assets/pdf (and covers to latex/images)"
	@echo "  make clean     - delete temp files in build/"
	@echo "  make distclean - delete build/"
	@echo "  make debug     - show detected files and dependencies"
