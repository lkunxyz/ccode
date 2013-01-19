--default smartc conf write with lua
--enjoy it.

server = {
	port = 80, -- default value is 80, type is integer
	inet4_addr = "0.0.0.0", --default value is '0.0.0.0', type is string
	--the size of cache file stay in /dev/shm. default size is 128m. support k, m, g.
	--type is string or integer. 
	max_size_in_shm = "128m", 
	--the size of cache file stay in user process mem. default size is 128m. support k, m, g.
	--type is string or integer.
	max_size_in_mem = "128m", 
}

--Store cache document in cache dir.
--Must configure cache_dir. Else cache server can not startup.
cache_dir = {
	[1] = {
		--path of dir 
		path = "/cache1",
		--max size of dir 
		max_size = "500G"
	},
	[2] = {
		path = "/cache2", 
		max_size = "500G"
	}
}

timeout = {
	--The time of client connection live in smartc. Default is 1 day. Type is string or integer
	--Support keywords s(secondes), m(minutes), h(hours), d(days)
	client_life = "1d", 
	--The time of server connection live in smartc. Default is 1 day. Type is string or integer
	--Support keywords s(secondes), m(minutes), h(hours), d(days)
	server_life = "1d",
	--The time of waiting for client request. Default is 60s. Type is string or integer.
	--Support keywords s(secondes), m(minutes), h(hours), d(days)
	request = 60, 
	--The time of waiting for server reply. Default is 60s. Type is string or integer.
	--Support keywords s(secondes), m(minutes), h(hours), d(days)
	reply = 60,
	--The time of connecting server. Default is 60s. Type is string or integer.
	--Support keywords s(secondes), m(minutes), h(hours), d(days)
	connect = 60
}
