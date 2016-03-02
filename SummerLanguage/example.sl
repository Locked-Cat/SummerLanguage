extern putc(c: char)->void
extern putd(d: double)->void
extern puts(s: string)->void
	

function main()->void
	var hello: string = "Hello World!" in
		puts(hello)