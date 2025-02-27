#include "offlinemsgmodel.hpp"
#include "db.hpp"


// 存储用户的离线消息
bool OfflineMsgModel::insert(int userid, string msg)
{
    // 组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "insert into offlinemessage values(%d, '%s')", userid, msg.c_str());

    Connection mysql_conn;
    if (mysql_conn.connect())
    {
        if (mysql_conn.update(sql))
        {
            return true;
        }
    }
    return false;
}

// 删除用户的离线消息
bool OfflineMsgModel::remove(int userid)
{
    // 组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "delete from offlinemessage where userid=%d", userid);

    Connection mysql_conn;
    if (mysql_conn.connect())
    {
        if (mysql_conn.update(sql))
        {
            return true;
        }
    }
    return false;
}

// 查询用户的离线消息
vector<string> OfflineMsgModel::query(int userid)
{
    // 组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "select message from offlinemessage where userid=%d", userid);

    vector<string> vec;
    Connection mysql_conn;
    if (mysql_conn.connect())
    {
        MYSQL_RES *res = mysql_conn.query(sql);
        if (res != nullptr)
        {
            // 把userid用户的所有离线消息放入vec中
            MYSQL_ROW row;
            while ((row = mysql_fetch_row(res)) != nullptr)
            {
                vec.push_back(row[0]);
            }
            mysql_free_result(res); // 释放结果集!!! 重要
            return vec;
        }
    }
    return vec;
}