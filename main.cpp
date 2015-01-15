#include "base/task_center.hpp"
#include "base/message_pump.hpp"
#include "base/task.h"
#include "base/singleton.h"

#include <iostream>
#include <string>


class TestClass
{
public:
    TestClass()
    {
        
    }
    ~TestClass()
    {
        
    }

    void Test()
    {
       std::cout<<"Call Test()"<<std::endl;
    }

    void test2(int a)
    {
        std::cout<< "int a = "<<a<<std::endl;
    }

    void Test3(std::string s)
    {
        std::cout<<s<<std::endl;
        base::Singleton<TaskCenterUI>::Instance().Quit(3);
    }

private:

};

int main()
{
    TestClass test_class;
    base::Task* task = base::NewMethodTask(&test_class, &TestClass::Test);

    base::Task* task2 = base::NewMethodTask(&test_class, &TestClass::test2, 10);

    std::string s = "1234 in main";
    base::Task* task3 = base::NewMethodTask(&test_class, &TestClass::Test3, s);

    
    base::Singleton<TaskCenterUI>::Instance().PostTask(task);
    base::Singleton<TaskCenterUI>::Instance().PostTask(task2);

    base::Singleton<TaskCenterUI>::Instance().PostTask(task3);
    base::Singleton<TaskCenterUI>::Instance().Run();

    ::getchar();
}