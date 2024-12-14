package gramfree

type DBMSType uint8
type DBMStatus uint8
type ClientP *Client

var (
	AFL_SHM_ID string
	MUT_SHM_ID string
)

const (
	// rdb login
	User = "fuzzer"
	Passwd = "goodluck"
	DBName = "fuzzdb"
	// redis
	RD_IP = "127.0.0.1"
	RD_PORT = "6379"
	// shared memory
	AFL_SHM_ENV = "SHM_ID"
	// dbms
	DBMS = "DBMS"
	PG = "postgresql"
	RD = "redis"
)

const (
	RDB DBMSType = iota
	NRDB
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