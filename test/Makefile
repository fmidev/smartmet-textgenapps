all: test
test: test_qdtext

.PHONY: test_qdtext

test_qdtext:
	./test_qdtext

# Extract only needed times to keep the file size small
fractiles_textgen.sqd:
	qdcrop -p $$(qdinfo -P -q /smartmet/data/climate/suomi/daily/querydata/fractiles_textgen.sqd | grep -v Min | grep Normal | grep F | awk '{print $$2}' | paste -sd ',') -S $$((find data -name '*.sqd' -exec qdpoint -p Helsinki -t UTC -q {} ';') | cut -f1 -d' ' | cut -b5-12 | sort | uniq | sed -e 's/^/2012/g' | paste -sd ',') /smartmet/data/climate/suomi/daily/querydata/fractiles_textgen.sqd fractiles_textgen.sqd

clean:
	rm -rf tmp
