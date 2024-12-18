package gramfree

type DBMStatus uint8
type ClientP *Client

const (
	// redis
	Redis_IP = "127.0.0.1"
	Redis_PORT = "6379"
	// shared memory
	AFL_SHM_ENV = "SHM_ID"
	// dbms
	DBMS = "DBMS"
	Redis = "redis"
	KeyDB = "keydb"
	MongoDB = "mongodb"
	AgensGraph = "agensgraph"
)

const (
	Crash DBMStatus = iota
	// execute error，maybe syntax or semantic error
	ExecError
	// connect error
	ConnectError
	Normal
)

type Client interface {
	// user passwd database
	Connect(string,string,string) bool
	Reconnect() bool
	Check_alive() bool
	Execute(string) DBMStatus
	Select_metadata() [][3]string
	Clean_up() bool
	Restart() bool
}