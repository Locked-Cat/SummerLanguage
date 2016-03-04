extern print_string(s: string)->void

function say(greeting: string)->void
begin
	print_string(greeting)
end

###################################################
## This is your first example of Summer Language ##
###################################################

function main()->void
begin
	var hello: string = "Hello, ", world: string = "world!\n" in 
	begin
		say(hello + world)
	end
end

main()
