extern print_number(num: number)->void	

function fib_iterate(n: number)->number
begin
	var a: number = 0, b: number = 1 in
	begin
		for i: number = 1, i <= n, 1 in
		begin
			b = a + b;
			a = b - a;
		end
		return a
	end
end

function fib_recursive(n: number)->number
begin
	var ret: number = 0 in
	begin
		if n <= 1 then 
		begin
			ret = n
		end
		else
		begin
			ret = fib_recursive(n - 1) + fib_recursive(n - 2)
		end
		return ret
	end
end

print_number(fib_iterate(42))
print_number(fib_recursive(42))