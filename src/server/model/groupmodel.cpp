#include "groupmodel.hpp"
#include "db.hpp"


// 创建群组
bool GroupModel::createGroup(Group &group)
{
    // 组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "insert into allgroup(groupname, groupdesc) values('%s', '%s')",
            group.getGroupName().c_str(), group.getGroupDesc().c_str());

    Connection mysql_conn;
    if (mysql_conn.connect())
    {
        if (mysql_conn.update(sql))
        {
            // 获取插入成功的用户数据生成的主键id
            group.setId(mysql_insert_id(mysql_conn.getConnection()));
            return true;
        }
    }
    return false;
}

// 加入群组
bool GroupModel::addGroup(int userid, int groupid, string role)
{
    // 组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "insert into groupuser values(%d, %d, '%s')", groupid, userid, role.c_str());

    Connection mysql;
    if (mysql.connect())
    {
        mysql.update(sql);
    }
    return true;
}

// 查询用户所在群组信息
vector<Group> GroupModel::queryGroups(int userid)
{
    // 查询用户所在的所有群组信息
    char sql[1024] = {0};
    sprintf(sql, "select a.id,a.groupname,a.groupdesc from allgroup a inner join groupuser b on a.id=b.groupid where b.userid=%d", userid);

    vector<Group> groupVec;
    Connection mysql;
    if (mysql.connect())
    {
        MYSQL_RES *res = mysql.query(sql);
        if (res != nullptr)
        {
            MYSQL_ROW row;
            while ((row = mysql_fetch_row(res)) != nullptr)
            {
                Group group;
                group.setId(atoi(row[0]));
                group.setGroupName(row[1]);
                group.setGroupDesc(row[2]);
                groupVec.push_back(group);
            }
            mysql_free_result(res);
        }
    }

    // 查询群组的用户信息
    for (Group &group : groupVec)
    {
        sprintf(sql, "select a.id,a.name,a.state,b.grouprole from user a inner join groupuser b on a.id=b.userid where b.groupid=%d", group.getId());
        MYSQL_RES *res = mysql.query(sql);
        if (res != nullptr)
        {
            MYSQL_ROW row;
            while ((row = mysql_fetch_row(res)) != nullptr)
            {
                GroupUser user;
                user.setId(atoi(row[0]));
                user.setName(row[1]);
                user.setState(row[2]);
                user.setRole(row[3]);
                group.getGroupUsers().push_back(user);
            }
            mysql_free_result(res);
        }
    }

    return groupVec;
}

//根据指定的groupid查询群组用户id列表，用于群组聊天业务，给群组中其他成员发消息
vector<int> GroupModel::queryGroupUsers(int userId, int groupId){
    char sql[1024] = {0};
    sprintf(sql, "select userid from groupuser where groupid=%d and userid!=%d", groupId, userId);

    vector<int> idVec;
    Connection mysql;
    if (mysql.connect())
    {
        MYSQL_RES *res = mysql.query(sql);
        if (res != nullptr)
        {
            MYSQL_ROW row;
            while ((row = mysql_fetch_row(res)) != nullptr)
            {
                idVec.push_back(atoi(row[0]));
            }
            mysql_free_result(res);
        }
    }
    return idVec;
}