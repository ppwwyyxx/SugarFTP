TARGET=report
TEX=xelatex -shell-escape
BIBTEX=biber
READER=evince

all: rebuild

rebuild output/$(TARGET).pdf: *.tex *.bib output
	cd output && rm -f *.tex *.bib && ln -s ../*.tex ../*.bib .
	cd output &&  $(TEX) $(TARGET).tex && $(BIBTEX) $(TARGET)
	cp output/$(TARGET).pdf ../ -f


output:
	mkdir output -p
	cd output && rm -f res img && ln -s ../src ../res .

view: output/$(TARGET).pdf
	$(READER) output/$(TARGET).pdf &

clean:
	rm -rf output

# compile twice to get cross reference right
final : *.tex *.bib output
	cd output && rm -f *.tex *.bib && ln -s ../*.tex ../*.bib .
	cd output && $(TEX) $(TARGET).tex && $(BIBTEX) $(TARGET) && \
		$(TEX) $(TARGET).tex && $(TEX) $(TARGET).tex

count:
	@word_count *.tex

run: view

.PHONY: all view clean rebuild run
