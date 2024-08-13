SCENE ?= cock.blend
OUTPUT ?= $(SCENE:.blend=.svg)
OPTS ?= --colormode bw --corner_threshold 30 --color_precision 8 \
        --filter_speckle 15 --gradient_step 0 --mode polygon

all: $(OUTPUT)

$(OUTPUT): $(SCENE)
	test -d anim || mkdir anim
	rm -f anim/frame*.{png,svg}
	xvfb-run --auto-servernum --server-args='-screen 0 1600x1024x16' \
		blender --background $^ --render-format PNG --render-output 'anim/frame####.png' \
		        --render-anim -noaudio
	# surround image with black border (passpartout) and convert to SVG
	for f in anim/frame*.png; do \
		echo "$$f -> $${f%.png}.svg"; \
		convert -bordercolor black -border 10 $$f $$f; \
		vtracer $(OPTS) --input $$f --output $${f%.png}.svg; \
	done
	python3 svg-anim.py --output $@ anim/

%: %.xz
	unxz --keep $^

clean:
	rm -f $(OUTPUT) $(SCENE)
	rm -rf anim/

.PHONY: all clean
