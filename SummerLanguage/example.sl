extern print_number(num: number)->void	

function main()->void
	for i: number = 0, i < 10, 2 in
		var j: number = i + 3 in
			print_number(j)

main()