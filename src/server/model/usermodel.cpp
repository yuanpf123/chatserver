#include "usermodel.hpp"
#include "db.hpp"
#include <memory>
#include <muduo/base/Logging.h>

static string ip = "127.0.0.1";
static unsigned short port = 3306;
static string user_name = "root";
static string passwd = "123456";
static string dbname = "chat";


bool UserModel::insert(User &user)
{
    // 组装sql语句
    char sql[1024]={0};
    sprintf(sql,
    "insert into user(name,password,state) values ('%s','%s','%s')",
    user.getName().c_str(),user.getPasswd().c_str(),user.getState().c_str());

    //声明自定义的mysql类
    Connection mysql_conn;
    // ConnectionPool* cp=ConnectionPool::getConnectionPool();
    // shared_ptr<Connection> sp=cp->getConnection();

    if(mysql_conn.connect(ip , port , user_name , passwd , dbname))
    {
        LOG_INFO << "数据库连接成功: ";
        if (mysql_conn.update(sql))
        {
            //mysql_insert_id 获取最后一次插入操作生成的自增 ID（如果表中有自增列的话）
            user.setId(mysql_insert_id(mysql_conn.getConnection()));
            return true;
        }
    }
    return false;
}

User UserModel::queryById(int id){
    char sql[1024] = {0};
    sprintf(sql, "select * from user where id = %d", id);

    Connection mysql_conn;
    if(mysql_conn.connect(ip, port, user_name, passwd, dbname)){
        MYSQL_RES *res = mysql_conn.query(sql);
        if(res != nullptr){
            //用主键查询，要么有一条记录，要么没有记录
            MYSQL_ROW row = mysql_fetch_row(res);
            if(row != nullptr){
                User user;
                user.setId(atoi(row[0])); //atoi将字符串转换为整数
                user.setName(row[1]);
                user.setPasswd(row[2]);
                user.setState(row[3]);
                mysql_free_result(res); //释放结果集 因为返回的是指针
                return user;
            }
        }
    }
    return User();
}

bool UserModel::updateState(User &user)
{
    char sql[1024] = {0};
    sprintf(sql, "update user set state = '%s' where id = %d", user.getState().c_str(), user.getId());

    Connection mysql_conn;
    if(mysql_conn.connect(ip, port, user_name, passwd, dbname)){
        if(mysql_conn.update(sql)){
            return true;
        }
    }
    return false;
}