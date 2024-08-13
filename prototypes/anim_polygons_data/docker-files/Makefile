SCENE ?= cock3_color.blend
OUTPUT ?= $(SCENE:.blend=.svg)
OPTS ?= --colormode bw --corner_threshold 30 --color_precision 8 \
        --filter_speckle 15 --gradient_step 0 --mode polygon

all: $(OUTPUT)

$(OUTPUT): anim/frame0000.png
	python3 png2svg.py --vtracer-opts "$(OPTS)" --output $@ anim/
	
anim/frame0000.png: $(SCENE)
	test -d anim || mkdir anim
	rm -f anim/frame*.png
	xvfb-run --auto-servernum --server-args='-screen 0 1600x1024x16' \
		blender --background $^ --render-format PNG --render-output 'anim/frame####.png' \
		        --render-anim -noaudio

%: %.xz
	unxz --keep $^

clean:
	rm -f $(OUTPUT) $(SCENE)
	rm -rf anim/

.PHONY: all clean
