
struct Test
{
    1:i32 id
    2:i32 value
}

service SrvTest
{
    void RpcTest(1:Test test)
}