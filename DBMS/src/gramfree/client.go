package gramfree

type DBMStatus uint8
type ClientP *Client

var (
	AFL_MAP_SIZE string
	AFL_SHM_ID string
)

const (
	// redis
	Redis_IP = "127.0.0.1"
	Redis_PORT = 6379
	// keydb
	KeyDB_PORT = 6380
	// memcached
	Memcached_PORT = 6381
	// dbms
	Redis = "redis"
	KeyDB = "keydb"
	Memcached = "memcached"
	Redis_Stack = "stack"
)

const (
	Crash DBMStatus = iota
	// execute error，maybe syntax or semantic error
	ExecError
	Normal
)

type Client interface {
	Connect(string,int) bool
	Reconnect() bool
	Check_alive() bool
	Execute(string) DBMStatus
	Collect_metadata() [][3]string
	Clean_up() bool
	Restart() bool
}