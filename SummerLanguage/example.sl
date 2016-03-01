extern putc(c)
extern putd(d)

function power(x, y)
	if y <> 0 then
		power(x, y - 1) * x
	else
		1

function binary@ 40(x, y)
	power(x, y)

function unary!(x)
	if x <> 0 then 
		0
	else
		1

putd(5 @ 3)
putc('\n')
putd(!4)
putc('\n')