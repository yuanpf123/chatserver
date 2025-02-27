#include "friendmodel.hpp"
#include "db.hpp"

// 添加好友关系
bool FriendModel::insert(int userid, int friendid)
{
    char sql[1024] = {0};
    sprintf(sql, "insert into friend(userid, friendid) values(%d, %d)", userid, friendid);

    Connection conn;
    if (conn.connect())
    {
        if (conn.update(sql))
        {
            return true;
        }
    }
    return false;
}

// 查询用户好友列表
vector<User> FriendModel::query(int userid)
{
    char sql[1024] = {0};
    sprintf(sql, "select a.id,a.name,a.state from user a inner join friend b on b.friendid=a.id where b.userid=%d", userid);

    vector<User> vec;
    Connection conn;
    if (conn.connect())
    {
        MYSQL_RES *res = conn.query(sql);
        if (res != nullptr)
        {
            // 把userid用户的所有好友信息添加到vec中
            MYSQL_ROW row;
            while ((row = mysql_fetch_row(res)) != nullptr)
            {
                User user;
                user.setId(atoi(row[0]));
                user.setName(row[1]);
                user.setState(row[2]);
                vec.push_back(user);
            }
            mysql_free_result(res);
        }
    }
    return vec;
}