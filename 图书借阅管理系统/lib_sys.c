#include <stdio.h>
#include <windows.h>
#include <string.h>
#include <stdlib.h>

#define READER_NUM 50     //读者人数上限
#define BOOK_NUM 100      //图书数量上限
#define RECORD_NUM 1000   //借阅数量上限
#define NAME_LEN 30       //姓名/书名最大长度
#define ID_LEN 20         //编号（图书/读者）最大长度
#define BORROW_ID_LEN 60  //借阅号长度（图书号+读者号+时间）
#define POS_X1 35         //菜单页第一列功能输出起始X坐标
#define POS_X2 40         //输入模块提示语句起始X坐标
#define POS_X3 50         //无数据时提示语起始X坐标
#define POS_X4 65         //菜单页第二列功能输出起始X坐标
#define POS_Y 3           //排序/提示输出起始Y坐标

//图书信息结构体
typedef struct book {
    char id[ID_LEN];        //图书编号
    char name[NAME_LEN];    //书名
    char author[NAME_LEN];  //作者
    char category[ID_LEN];  //分类号
    char publisher[NAME_LEN];//出版单位
    char publishTime[20];   //出版时间
    int stock;              //库存数量
    float price;            //价格
    int borrowCount;        //借阅次数
} BOOK;

//读者信息结构体（新增借阅号数组，存储自己的借阅凭证）
typedef struct reader {
    char id[ID_LEN];                //读者编号
    char password[ID_LEN];          //独立登录密码
    char name[NAME_LEN];            //姓名
    int maxBorrow;                  //最大借阅额度
    int borrowed;                   //已借阅数量
    int isAdmin;                    //是否为管理员 0-读者 1-管理员
    char borrowIds[READER_NUM][BORROW_ID_LEN]; // 借阅号数组（1个读者最多READER_NUM个借阅号）
    int borrowIdCount;              // 当前借阅号数量（未归还）
} READER;

//借阅记录结构
typedef struct borrowRecord {
    char bookId[ID_LEN];    //图书编号
    char readerId[ID_LEN];  //读者编号
    char borrowTime[20];    //借阅时间
    char returnTime[20];    //归还时间
    int isReturned;         //是否归还 0-未还 1-已还
} BORROW_RECORD;

//全局变量
BOOK books[BOOK_NUM];               //图书数组
READER readers[READER_NUM];         //读者数组
BORROW_RECORD records[RECORD_NUM];    //借阅记录数组
int bookCount = 0;                  //当前图书数量
int readerCount = 0;                //当前读者数量
int recordCount = 0;                //当前借阅记录数量

//自定义函数声明（已移除IsDataChanged）
void AdminMenu(READER curUser);                                                    //管理员菜单（内置功能分支）
void ReaderMenu(READER curUser);                                                    //读者菜单（内置功能分支）
void SetPosition(int x, int y);                                                   //设置控制台输出位置
void BatchImportBook(int *n, BOOK book[]);                                        //批量导入图书信息
void BatchImportReader(int *n, READER reader[]);                                   //批量导入读者信息
void AppendBook(int *n, BOOK book[]);                                             //增加图书信息
void AppendReader(int *n, READER reader[]);                                       //增加读者信息
void DeleteBook(int *n, BOOK book[]);                                             //删除图书信息
void DeleteReader(int *n, READER reader[]);                                       //删除读者信息
void SearchBookById(int n, BOOK book[]);                                          //按图书编号查询
void SearchReaderById(int n, READER reader[]);                                    //按读者编号查询
void ModifyBook(int n, BOOK book[]);                                              //修改图书信息
void ModifyReader(int n, READER reader[]);                                        //修改读者信息
void BorrowBook(READER curUser);                                                  //图书借阅
void ReturnBook(READER curUser);                                                  //图书归还
void QueryBorrowRecord(int n, BORROW_RECORD record[]);                            //按图书/读者编号查询借阅记录
void QueryBorrowByAuthor(int n, BORROW_RECORD record[], BOOK books[]);            //按作者查询借阅记录
void SearchBookByNameOrAuthor(int n, BOOK book[]);                                //按图书名称/作者查询图书
void QueryMyRecord(char *readerId);                                               //查询个人借阅记录
void StatBorrowTop10(int n, BOOK book[]);                                         //借阅次数TOP10统计
void PrintBook(int n, BOOK book[]);                                               //打印图书信息
void PrintReader(int n, READER reader[]);                                         //打印读者信息
void ShowPersonalInfo(READER user);                                               //查看个人信息（含借阅号）
void ModifyPersonalInfo(READER *curUser);                                          //读者修改个人信息（姓名+密码）
void WritetoFile();                                                               //数据存盘
int ReadfromFile();                                                               //从文件读取数据
READER Login();                                                                   //登录验证
void GenerateBorrowId(char *borrowId, char *bookId, char *readerId, char *time); //生成借阅号

int main() {

    READER curUser = {"", "", "", 0, 0, -1, {{0}}, 0}; //当前登录用户（初始化借阅号数组）
    system("mode con cols=130 lines=60");
    system("color 0E");

    // 程序启动时自动读取历史数据
    if (ReadfromFile() != 0) {
        SetPosition(POS_X3, POS_Y);
        printf("无历史数据，初始化默认管理员！\n");
        // 初始化默认管理员（仅首次无数据时）
        strcpy(readers[0].id, "admin");
        strcpy(readers[0].password, "admin123");
        strcpy(readers[0].name, "系统管理员");
        readers[0].maxBorrow = 0;
        readers[0].borrowed = 0;
        readers[0].isAdmin = 1;
        readers[0].borrowIdCount = 0; // 管理员无借阅权限，借阅号数量为0
        memset(readers[0].borrowIds, 0, sizeof(readers[0].borrowIds)); // 清空借阅号数组
        readerCount = 1;
        WritetoFile(); // 保存默认管理员
    }

    while (1) {
        system("cls");
        //未登录时先显示登录菜单
        if (curUser.isAdmin == -1) {
            curUser = Login();
            if (curUser.isAdmin == -1) {
                SetPosition(POS_X3, POS_Y);
                printf("登录失败，按任意键重试！\n");
                getchar();
                continue;
            }
            SetPosition(POS_X3, POS_Y);
            printf("%s登录成功，按任意键进入系统！\n", curUser.isAdmin ? "管理员" : "读者");
            getchar();
        }

        //根据用户类型进入对应菜单（内置功能分支）
        system("cls");
        if (curUser.isAdmin == 1) {
            AdminMenu(curUser);
        } else {
            ReaderMenu(curUser);
        }
    }
    return 0;
}

//设置控制台输出位置
void SetPosition(int x, int y) {
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    COORD pos = {x, y};
    SetConsoleCursorPosition(hOut, pos);
}

//生成借阅号（图书号+读者号+时间）
void GenerateBorrowId(char *borrowId, char *bookId, char *readerId, char *time) {
    strcpy(borrowId, bookId);
    strcat(borrowId, "_");
    strcat(borrowId, readerId);
    strcat(borrowId, "_");
    strcat(borrowId, time);
}

//管理员菜单（内置功能分支，修改菜单名称）
void AdminMenu(READER curUser) {
    int option, posy = 5;
    int i, j;

    SetPosition(POS_X3, posy);
    printf("图书借阅管理系统（管理员端）\n");
    for (i = 0; i < 2; i++) {
        SetPosition(POS_X1, ++posy);
        for (j = 0; j < 55; j++) printf("-");
    }

    // 菜单名称修改：输入 → 批量导入
    SetPosition(POS_X1, ++posy); printf("1. 批量导入图书信息");
    SetPosition(POS_X4, posy);   printf("2. 批量导入读者信息");
    SetPosition(POS_X1, posy += 2); printf("3. 增加图书信息");
    SetPosition(POS_X4, posy);   printf("4. 增加读者信息");
    SetPosition(POS_X1, posy += 2); printf("5. 删除图书信息");
    SetPosition(POS_X4, posy);   printf("6. 删除读者信息");
    SetPosition(POS_X1, posy += 2); printf("7. 查询图书");
    SetPosition(POS_X4, posy);   printf("8. 按编号查询读者");
    SetPosition(POS_X1, posy += 2); printf("9. 修改图书信息");
    SetPosition(POS_X4, posy);   printf("10. 修改读者信息");
    SetPosition(POS_X1, posy += 2); printf("11. 查询借阅记录");
    SetPosition(POS_X4, posy);   printf("12. 借阅次数TOP10");
    SetPosition(POS_X1, posy += 2); printf("13. 打印图书信息");
    SetPosition(POS_X4, posy);   printf("14. 打印读者信息");
    SetPosition(POS_X1, posy += 2); printf("0. 退出系统");

    for (i = 0; i < 2; i++) {
        SetPosition(POS_X1, ++posy);
        for (j = 0; j < 55; j++) printf("-");
    }

    SetPosition(POS_X1, ++posy);
    printf("请选择操作[0~16]: ");
    scanf("%d", &option);

    //内置功能分支
    switch (option) {
        case 1:
            system("cls");
            BatchImportBook(&bookCount, books); 
            WritetoFile(); // 导入后自动保存
            system("pause");
            break;
        case 2:
            system("cls");
            BatchImportReader(&readerCount, readers); 
            WritetoFile(); // 导入后自动保存
            system("pause");
            break;
        case 3:
            system("cls");
            AppendBook(&bookCount, books);
            WritetoFile(); // 增加后自动保存
            system("pause");
            break;
        case 4:
            system("cls");
            AppendReader(&readerCount, readers);
            WritetoFile(); // 增加后自动保存
            system("pause");
            break;
        case 5:
            system("cls");
            if (bookCount == 0) { 
                SetPosition(POS_X3, POS_Y); 
                printf("无图书信息，请先导入/增加！\n"); 
                system("pause"); 
                break; 
            }
            DeleteBook(&bookCount, books);
            WritetoFile(); // 删除后自动保存
            system("pause");
            break;
        case 6:
            system("cls");
            if (readerCount == 0) { 
                SetPosition(POS_X3, POS_Y); 
                printf("无读者信息，请先导入/增加！\n"); 
                system("pause"); 
                break; 
            }
            DeleteReader(&readerCount, readers);
            WritetoFile(); // 删除后自动保存
            system("pause");
            break;
        case 7:
            system("cls");
            if (bookCount == 0) { 
                SetPosition(POS_X3, POS_Y); 
                printf("无图书信息，请先导入/增加！\n"); 
                system("pause"); 
                break; 
            }
            int readerQueryType;
            SetPosition(POS_X2, 5);
            printf("1. 按图书编号查询  2. 按名称/作者查询\n");
            SetPosition(POS_X2, 7);
            printf("选择查询方式：");
            scanf("%d", &readerQueryType);
            if (readerQueryType == 1) {
                SearchBookById(bookCount, books);
            } else if (readerQueryType == 2) {
                SearchBookByNameOrAuthor(bookCount, books);
            } else {
                SetPosition(POS_X3, POS_Y);
                printf("输入错误！\n");
            }
            system("pause");
            break;
        case 8:
            system("cls");
            if (readerCount == 0) { 
                SetPosition(POS_X3, POS_Y); 
                printf("无读者信息，请先导入/增加！\n"); 
                system("pause"); 
                break; 
            }
            SearchReaderById(readerCount, readers);
            system("pause");
            break;
        case 9:
            system("cls");
            if (bookCount == 0) { 
                SetPosition(POS_X3, POS_Y); 
                printf("无图书信息，请先导入/增加！\n"); 
                system("pause"); 
                break; 
            }
            ModifyBook(bookCount, books);
            WritetoFile(); // 修改后自动保存
            system("pause");
            break;
        case 10:
            system("cls");
            if (readerCount == 0) { 
                SetPosition(POS_X3, POS_Y); 
                printf("无读者信息，请先导入/增加！\n"); 
                system("pause"); 
                break; 
            }
            ModifyReader(readerCount, readers);
            WritetoFile(); // 修改后自动保存
            system("pause");
            break;
        case 11:
            system("cls");
            if (recordCount == 0) { 
                SetPosition(POS_X3, POS_Y); 
                printf("无借阅记录，请先操作！\n"); 
                system("pause"); 
                break; 
            }
            // 管理员选择借阅查询方式
            int queryType;
            SetPosition(POS_X2, 5);
            printf("1. 按图书/读者编号查询  2. 按作者查询\n");
            SetPosition(POS_X2, 7);
            printf("选择查询方式：");
            scanf("%d", &queryType);
            if (queryType == 1) {
                QueryBorrowRecord(recordCount, records);
            } else if (queryType == 2) {
                QueryBorrowByAuthor(recordCount, records, books);
            } else {
                SetPosition(POS_X3, POS_Y);
                printf("输入错误！\n");
            }
            system("pause");
            break;
        case 12:
            system("cls");
            if (bookCount == 0) { 
                SetPosition(POS_X3, POS_Y); 
                printf("无图书信息，请先导入/增加！\n"); 
                system("pause"); 
                break; 
            }
            StatBorrowTop10(bookCount, books);
            system("pause");
            break;
        case 13:
            system("cls");
            if (bookCount == 0) { 
                SetPosition(POS_X3, POS_Y); 
                printf("无图书信息，请先导入/增加！\n"); 
                system("pause"); 
                break; 
            }
            PrintBook(bookCount, books);
            system("pause");
            break;
        case 14:
            system("cls");
            if (readerCount == 0) { 
                SetPosition(POS_X3, POS_Y); 
                printf("无读者信息，请先导入/增加！\n"); 
                system("pause"); 
                break; 
            }
            PrintReader(readerCount, readers);
            system("pause");
            break;
        case 0:
            system("cls");
            WritetoFile(); // 退出前自动保存所有数据
            SetPosition(POS_X3, POS_Y);
            printf("退出系统，感谢使用！\n");
            system("pause");
            exit(0);
            break;
        default:
            system("cls");
            SetPosition(POS_X3, POS_Y);
            printf("输入错误，按任意键重试！\n");
            system("pause");
            break;
    }
}

//读者菜单
void ReaderMenu(READER curUser) {
    int option, posy = 5;
    int i, j;

    SetPosition(POS_X3, posy);
    printf("图书借阅管理系统（读者端）\n");
    for (i = 0; i < 2; i++) {
        SetPosition(POS_X1, ++posy);
        for (j = 0; j < 55; j++) printf("-");
    }

    SetPosition(POS_X1, ++posy); printf("1. 查询图书");
    SetPosition(POS_X4, posy);   printf("2. 借阅图书");
    SetPosition(POS_X1, posy += 2); printf("3. 归还图书");
    SetPosition(POS_X4, posy);   printf("4. 查询我的借阅记录");
    SetPosition(POS_X1, posy += 2); printf("5. 查看个人信息");
    SetPosition(POS_X4, posy);   printf("6. 修改个人信息");
    SetPosition(POS_X1, posy += 2); printf("0. 退出登录");

    for (i = 0; i < 2; i++) {
        SetPosition(POS_X1, ++posy);
        for (j = 0; j < 55; j++) printf("-");
    }

    SetPosition(POS_X1, ++posy);
    printf("请选择操作[0~6]: ");
    scanf("%d", &option);

    //内置功能分支，无偏移
    switch (option) {
        case 1:
            system("cls");
            if (bookCount == 0) {
                SetPosition(POS_X3, POS_Y);
                printf("无图书信息！\n");
                system("pause");
                break;
            }
            // 读者选择图书查询方式
            int readerQueryType;
            SetPosition(POS_X2, 5);
            printf("1. 按图书编号查询  2. 按名称/作者查询\n");
            SetPosition(POS_X2, 7);
            printf("选择查询方式：");
            scanf("%d", &readerQueryType);
            if (readerQueryType == 1) {
                SearchBookById(bookCount, books);
            } else if (readerQueryType == 2) {
                SearchBookByNameOrAuthor(bookCount, books);
            } else {
                SetPosition(POS_X3, POS_Y);
                printf("输入错误！\n");
            }
            system("pause");
            break;
        case 2:
            system("cls");
            if (bookCount == 0) {
                SetPosition(POS_X3, POS_Y);
                printf("无图书信息！\n");
                system("pause");
                break;
            }
            BorrowBook(curUser);
            WritetoFile(); // 借阅图书后自动保存
            system("pause");
            break;
        case 3:
            system("cls");
            if (recordCount == 0) {
                SetPosition(POS_X3, POS_Y);
                printf("无借阅记录！\n");
                system("pause");
                break;
            }
            ReturnBook(curUser);
            WritetoFile(); // 归还图书后自动保存
            system("pause");
            break;
        case 4:
            system("cls");
            QueryMyRecord(curUser.id);
            system("pause");
            break;
        case 5:
            system("cls");
            ShowPersonalInfo(curUser);
            system("pause");
            break;
        case 6:
            system("cls");
            ModifyPersonalInfo(&curUser);
            WritetoFile(); // 修改个人信息后自动保存
            system("pause");
            break;
        case 0:
            system("cls");
            WritetoFile(); // 退出前自动保存所有数据
            SetPosition(POS_X3, POS_Y);
            printf("退出系统，感谢使用！\n");
            system("pause");
            exit(0);
            break;
        default:
            system("cls");
            SetPosition(POS_X3, POS_Y);
            printf("输入错误，按任意键重试！\n");
            system("pause");
            break;
    }
}

//登录验证（验证编号+独立密码）
READER Login() {
    char id[ID_LEN], pwd[ID_LEN];
    int i;
    READER empty = {"", "", "", 0, 0, -1, {{0}}, 0};

    system("cls");
    SetPosition(POS_X2, 5);
    printf("===== 系统登录 =====\n");
    SetPosition(POS_X2, 7);
    printf("输入读者/管理员编号：");
    scanf("%s", id);
    SetPosition(POS_X2, 9);
    printf("输入登录密码：");
    scanf("%s", pwd);

    // 验证用户
    for (i = 0; i < readerCount; i++) {
        if (strcmp(readers[i].id, id) == 0 && strcmp(readers[i].password, pwd) == 0) {
            return readers[i];
        }
    }
    return empty;
}

//读者修改个人信息（姓名+密码）
void ModifyPersonalInfo(READER *curUser) {
    char oldPwd[ID_LEN], newPwd[ID_LEN], newName[NAME_LEN];
    int i, posy = 6;

    SetPosition(POS_X2, posy);
    printf("===== 修改个人信息 =====\n");
    SetPosition(POS_X2, ++posy);
    printf("输入当前密码：");
    scanf("%s", oldPwd);

    //验证当前密码
    if (strcmp(oldPwd, curUser->password) != 0) {
        SetPosition(POS_X2, ++posy);
        printf("密码错误！修改失败！\n");
        return;
    }

    //修改姓名
    SetPosition(POS_X2, ++posy);
    printf("输入新姓名：");
    scanf("%s", newName);
    strcpy(curUser->name, newName);

    //修改密码
    SetPosition(POS_X2, ++posy);
    printf("输入新密码：");
    scanf("%s", newPwd);
    strcpy(curUser->password, newPwd);

    //同步更新读者数组中的信息
    for (i = 0; i < readerCount; i++) {
        if (strcmp(readers[i].id, curUser->id) == 0) {
            strcpy(readers[i].name, newName);
            strcpy(readers[i].password, newPwd);
            break;
        }
    }

    SetPosition(POS_X2, ++posy);
    printf("姓名和密码修改成功！\n");
}

//批量导入图书信息
void BatchImportBook(int *n, BOOK book[]) {
    int i, posy = 6;
    // 防覆盖提示（强化批量导入的风险提示）
    if (*n > 0) {
        SetPosition(POS_X2, posy);
        printf("批量导入警告：当前已有 %d 本图书，导入会覆盖所有原有图书数据！\n", *n);
        SetPosition(POS_X2, posy+1);
        printf("该功能仅用于系统初始化批量导入，日常新增请使用「增加图书信息」！\n");
        SetPosition(POS_X2, posy+2);
        printf("确认继续？(Y/N)：");
        char ch;
        getchar();
        scanf("%c", &ch);
        if (ch != 'Y' && ch != 'y') {
            printf("操作取消！\n");
            return;
        }
        posy += 3;
    }

    system("cls");
    SetPosition(POS_X2, posy);
    printf("===== 批量导入图书信息（系统初始化专用）=====\n");
    SetPosition(POS_X2, ++posy);
    printf("输入要导入的图书数量(n<%d)：", BOOK_NUM);
    scanf("%d", n);
    if (*n > BOOK_NUM) {
        SetPosition(POS_X2, posy + 2);
        printf("数量超出上限，自动设为%d！\n", BOOK_NUM);
        *n = BOOK_NUM;
    }

    for (i = 0; i < 2; i++) {
        SetPosition(POS_X1, ++posy);
        for (int j = 0; j < 55; j++) printf("-");
    }

    SetPosition(POS_X2, ++posy);
    printf("请批量输入图书信息（每行格式：编号 书名 作者 分类号 出版社 出版时间 库存 价格）：\n");
    for (i = 0; i < *n; i++) {
        SetPosition(POS_X2, ++posy);
        printf("第%d本：", i + 1);
        scanf("%s %s %s %s %s %s %d %f",
              book[i].id, book[i].name, book[i].author, book[i].category,
              book[i].publisher, book[i].publishTime, &book[i].stock, &book[i].price);
        book[i].borrowCount = 0;
    }
    SetPosition(POS_X2, ++posy);
    printf("批量导入图书信息完成！共导入 %d 本图书\n", *n);
}

//批量导入读者信息（原InputReader，改名+强化批量定位）
void BatchImportReader(int *n, READER reader[]) {
    int i, posy = 6;
    // 防覆盖提示（强化批量导入的风险提示）
    if (*n > 0) {
        SetPosition(POS_X2, posy);
        printf("批量导入警告：当前已有 %d 个读者或管理员，导入会覆盖所有原有读者数据！\n", *n);
        SetPosition(POS_X2, posy+1);
        printf("该功能仅用于系统初始化批量导入，日常新增请使用「增加读者信息」！\n");
        SetPosition(POS_X2, posy+2);
        printf("请保证批量导入的读者数据存在管理员，否则将导致管理员功能缺失！\n");
        SetPosition(POS_X2, posy+3);
        printf("确认继续？(Y/N)：");
        char ch;
        getchar();
        scanf("%c", &ch);
        if (ch != 'Y' && ch != 'y') {
            printf("操作取消！\n");
            return;
        }
        posy += 3;
    }

    system("cls");
    SetPosition(POS_X2, posy);
    printf("===== 批量导入读者信息（系统初始化专用）=====\n");
    SetPosition(POS_X2, ++posy);
    printf("输入要导入的读者数量(n<%d)：", READER_NUM);
    scanf("%d", n);
    if (*n > READER_NUM) {
        SetPosition(POS_X2, posy + 2);
        printf("数量超出上限，自动设为%d！\n", READER_NUM);
        *n = READER_NUM;
    }

    for (i = 0; i < 2; i++) {
        SetPosition(POS_X1, ++posy);
        for (int j = 0; j < 55; j++) printf("-");
    }

    SetPosition(POS_X2, ++posy);
    printf("请批量输入读者信息（每行格式：编号 密码 姓名 最大借阅量 是否为管理员0-读者1-管理员）：\n");
    for (i = 0; i < *n; i++) {
        SetPosition(POS_X2, ++posy);
        printf("第%d个：", i + 1);
        scanf("%s %s %s %d %d",
              reader[i].id, reader[i].password, reader[i].name, &reader[i].maxBorrow, &reader[i].isAdmin);
        reader[i].borrowed = 0;
        reader[i].borrowIdCount = 0; // 初始化借阅号数量
        memset(reader[i].borrowIds, 0, sizeof(reader[i].borrowIds)); // 清空借阅号数组
    }
    SetPosition(POS_X2, ++posy);
    printf("批量导入读者信息完成！共导入 %d 个读者\n", *n);
}

//增加图书信息（日常增量添加，无修改）
void AppendBook(int *n, BOOK book[]) {
    int num, i, posy = 6;
    SetPosition(POS_X2, posy);
    printf("===== 增加图书信息（日常新增专用）=====\n");
    SetPosition(POS_X2, ++posy);
    printf("输入要增加的图书数量：");
    scanf("%d", &num);

    if (*n + num > BOOK_NUM) {
        SetPosition(POS_X2, posy + 2);
        printf("数量超出上限，最多可增加%d本！\n", BOOK_NUM - *n);
        num = BOOK_NUM - *n;
    }

    for (i = *n; i < *n + num; i++) {
        SetPosition(POS_X2, ++posy);
        printf("输入第%d本图书信息（编号 书名 作者 分类号 出版社 出版时间 库存 价格）：\n", i + 1);
        SetPosition(POS_X2, posy + 1);
        scanf("%s %s %s %s %s %s %d %f",
              book[i].id, book[i].name, book[i].author, book[i].category,
              book[i].publisher, book[i].publishTime, &book[i].stock, &book[i].price);
        book[i].borrowCount = 0;
        posy++;
    }
    *n += num;
    SetPosition(POS_X2, ++posy);
    printf("图书增加完毕！当前总图书数：%d\n", *n);
}

//增加读者信息（日常增量添加，无修改）
void AppendReader(int *n, READER reader[]) {
    int num, i, posy = 6;
    SetPosition(POS_X2, posy);
    printf("===== 增加读者信息（日常新增专用）=====\n");
    SetPosition(POS_X2, ++posy);
    printf("输入要增加的读者数量：");
    scanf("%d", &num);

    if (*n + num > READER_NUM) {
        SetPosition(POS_X2, posy + 2);
        printf("数量超出上限，最多可增加%d个！\n", READER_NUM - *n);
        num = READER_NUM - *n;
    }

    for (i = *n; i < *n + num; i++) {
        SetPosition(POS_X2, ++posy);
        printf("输入第%d个读者信息（编号 密码 姓名 最大借阅量 是否为管理员0-读者1-管理员）：\n", i + 1);
        SetPosition(POS_X2, posy + 1);
        scanf("%s %s %s %d %d",
              reader[i].id, reader[i].password, reader[i].name, &reader[i].maxBorrow, &reader[i].isAdmin);
        reader[i].borrowed = 0;
        reader[i].borrowIdCount = 0; // 初始化借阅号数量
        memset(reader[i].borrowIds, 0, sizeof(reader[i].borrowIds)); // 清空借阅号数组
        posy++;
    }
    *n += num;
    SetPosition(POS_X2, ++posy);
    printf("读者增加完毕！当前总读者数：%d\n", *n);
}

//删除图书信息
void DeleteBook(int *n, BOOK book[]) {
    char delId[ID_LEN];
    int i, j, flag = 0, posy = 6;
    SetPosition(POS_X2, posy);
    printf("输入要删除的图书编号：");
    scanf("%s", delId);

    for (i = 0; i < *n; i++) {
        if (strcmp(book[i].id, delId) == 0) {
            flag = 1;
            SetPosition(POS_X2, ++posy);
            printf("找到图书：%s %s %s 库存：%d\n", book[i].id, book[i].name, book[i].author, book[i].stock);
            SetPosition(POS_X2, ++posy);
            printf("确认删除？(Y/N)：");
            getchar();
            char ch;
            scanf("%c", &ch);

            if (ch == 'Y' || ch == 'y') {
                for (j = i; j < *n - 1; j++) {
                    book[j] = book[j + 1];
                }
                (*n)--;
                SetPosition(POS_X2, ++posy);
                printf("删除成功！\n");
            } else {
                SetPosition(POS_X2, ++posy);
                printf("取消删除！\n");
            }
            break;
        }
    }

    if (!flag) {
        SetPosition(POS_X2, ++posy);
        printf("未找到该图书！\n");
    }
}

//删除读者信息
void DeleteReader(int *n, READER reader[]) {
    char delId[ID_LEN];
    int i, j, flag = 0, posy = 6;
    SetPosition(POS_X2, posy);
    printf("输入要删除的读者编号：");
    scanf("%s", delId);

    for (i = 0; i < *n; i++) {
        if (strcmp(reader[i].id, delId) == 0) {
            flag = 1;
            SetPosition(POS_X2, ++posy);
            printf("找到读者：%s %s 最大借阅量：%d\n", reader[i].id, reader[i].name, reader[i].maxBorrow);
            SetPosition(POS_X2, ++posy);
            printf("确认删除？(Y/N)：");
            getchar();
            char ch;
            scanf("%c", &ch);

            if (ch == 'Y' || ch == 'y') {
                for (j = i; j < *n - 1; j++) {
                    reader[j] = reader[j + 1];
                }
                (*n)--;
                SetPosition(POS_X2, ++posy);
                printf("删除成功！\n");
            } else {
                SetPosition(POS_X2, ++posy);
                printf("取消删除！\n");
            }
            break;
        }
    }

    if (!flag) {
        SetPosition(POS_X2, ++posy);
        printf("未找到该读者！\n");
    }
}

//按图书编号查询
void SearchBookById(int n, BOOK book[]) {
    char id[ID_LEN];
    int i, flag = 0, posy = 6;
    system("cls");
    SetPosition(POS_X2, posy);
    printf("输入要查询的图书编号：");
    scanf("%s", id);

    SetPosition(POS_X1, ++posy);
    printf("===== 查询结果 =====\n");
    for (i = 0; i < n; i++) {
        if (strcmp(book[i].id, id) == 0) {
            flag = 1;
            SetPosition(POS_X2, ++posy);
            printf("编号：%s\n", book[i].id);
            SetPosition(POS_X2, posy + 1);
            printf("书名：%s\n", book[i].name);
            SetPosition(POS_X2, posy + 2);
            printf("作者：%s\n", book[i].author);
            SetPosition(POS_X2, posy + 3);
            printf("分类号：%s\n", book[i].category);
            SetPosition(POS_X2, posy + 4);
            printf("出版社：%s\n", book[i].publisher);
            SetPosition(POS_X2, posy + 5);
            printf("出版时间：%s\n", book[i].publishTime);
            SetPosition(POS_X2, posy + 6);
            printf("库存：%d\n", book[i].stock);
            SetPosition(POS_X2, posy + 7);
            printf("价格：%.2f\n", book[i].price);
            SetPosition(POS_X2, posy + 8);
            printf("借阅次数：%d\n", book[i].borrowCount);
            posy += 8;
            break;
        }
    }

    if (!flag) {
        SetPosition(POS_X2, ++posy);
        printf("未找到该图书！\n");
    }
}

//按读者编号查询（含借阅号）
void SearchReaderById(int n, READER reader[]) {
    char id[ID_LEN];
    int i, flag = 0, posy = 6;
    SetPosition(POS_X2, posy);
    printf("输入要查询的读者编号：");
    scanf("%s", id);

    SetPosition(POS_X1, ++posy);
    printf("===== 查询结果 =====\n");
    for (i = 0; i < n; i++) {
        if (strcmp(reader[i].id, id) == 0) {
            flag = 1;
            SetPosition(POS_X2, ++posy);
            printf("编号：%s\n", reader[i].id);
            SetPosition(POS_X2, posy + 1);
            printf("姓名：%s\n", reader[i].name);
            SetPosition(POS_X2, posy + 2);
            printf("最大借阅量：%d\n", reader[i].maxBorrow);
            SetPosition(POS_X2, posy + 3);
            printf("已借阅数量：%d\n", reader[i].borrowed);
            SetPosition(POS_X2, posy + 4);
            printf("身份：%s\n", reader[i].isAdmin ? "管理员" : "普通读者");
            // 展示借阅号
            SetPosition(POS_X2, posy + 5);
            printf("当前借阅号：\n");
            if (reader[i].borrowIdCount == 0) {
                SetPosition(POS_X2, posy + 6);
                printf("  无\n");
            } else {
                for (int j = 0; j < reader[i].borrowIdCount; j++) {
                    SetPosition(POS_X2, posy + 6 + j);
                    printf("  %s\n", reader[i].borrowIds[j]);
                }
            }
            posy += 5 + reader[i].borrowIdCount;
            break;
        }
    }

    if (!flag) {
        SetPosition(POS_X2, ++posy);
        printf("未找到该读者！\n");
    }
}

//修改图书信息
void ModifyBook(int n, BOOK book[]) {
    char modId[ID_LEN];
    int i, flag = 0, posy = 6;
    SetPosition(POS_X2, posy);
    printf("输入要修改的图书编号：");
    scanf("%s", modId);

    for (i = 0; i < n; i++) {
        if (strcmp(book[i].id, modId) == 0) {
            flag = 1;
            SetPosition(POS_X1, ++posy);
            printf("===== 原图书信息 =====\n");
            SetPosition(POS_X2, ++posy);
            printf("编号：%s 书名：%s 作者：%s 库存：%d 价格：%.2f\n",
                   book[i].id, book[i].name, book[i].author, book[i].stock, book[i].price);

            SetPosition(POS_X1, ++posy);
            printf("===== 输入新信息 =====\n");
            SetPosition(POS_X2, ++posy);
            printf("新书名：");
            scanf("%s", book[i].name);
            SetPosition(POS_X2, ++posy);
            printf("新作者：");
            scanf("%s", book[i].author);
            SetPosition(POS_X2, ++posy);
            printf("新库存：");
            scanf("%d", &book[i].stock);
            SetPosition(POS_X2, ++posy);
            printf("新价格：");
            scanf("%f", &book[i].price);

            SetPosition(POS_X2, ++posy);
            printf("修改成功！\n");
            break;
        }
    }

    if (!flag) {
        SetPosition(POS_X2, ++posy);
        printf("未找到该图书！\n");
    }
}

//修改读者信息（管理员用）
void ModifyReader(int n, READER reader[]) {
    char modId[ID_LEN];
    int i, flag = 0, posy = 6;
    SetPosition(POS_X2, posy);
    printf("输入要修改的读者编号：");
    scanf("%s", modId);

    for (i = 0; i < n; i++) {
        if (strcmp(reader[i].id, modId) == 0) {
            flag = 1;
            SetPosition(POS_X1, ++posy);
            printf("===== 原读者信息 =====\n");
            SetPosition(POS_X2, ++posy);
            printf("编号：%s 姓名：%s 最大借阅量：%d\n",
                   reader[i].id, reader[i].name, reader[i].maxBorrow);

            SetPosition(POS_X1, ++posy);
            printf("===== 输入新信息 =====\n");
            SetPosition(POS_X2, ++posy);
            printf("新姓名：");
            scanf("%s", reader[i].name);
            SetPosition(POS_X2, ++posy);
            printf("新密码：");
            scanf("%s", reader[i].password);
            SetPosition(POS_X2, ++posy);
            printf("新最大借阅量：");
            scanf("%d", &reader[i].maxBorrow);

            SetPosition(POS_X2, ++posy);
            printf("修改成功！\n");
            break;
        }
    }

    if (!flag) {
        SetPosition(POS_X2, ++posy);
        printf("未找到该读者！\n");
    }
}

//图书借阅（生成借阅号并关联到读者结构体）
void BorrowBook(READER curUser) {
    char bookId[ID_LEN], borrowTime[20];
    char borrowId[BORROW_ID_LEN]; // 临时存储生成的借阅号
    int i, flag = 0, posy = 6;
    SetPosition(POS_X2, posy);
    printf("输入要借阅的图书编号：");
    scanf("%s", bookId);
    SetPosition(POS_X2, ++posy);
    printf("输入借阅时间（YYYYMMDD）：");
    scanf("%s", borrowTime);

    for (i = 0; i < bookCount; i++) {
        if (strcmp(books[i].id, bookId) == 0) {
            flag = 1;
            if (books[i].stock <= 0) {
                SetPosition(POS_X2, ++posy);
                printf("库存不足，无法借阅！\n");
                return;
            }
            if (curUser.borrowed >= curUser.maxBorrow) {
                SetPosition(POS_X2, ++posy);
                printf("已达最大借阅量，无法借阅！\n");
                return;
            }

            // 生成借阅号
            GenerateBorrowId(borrowId, books[i].id, curUser.id, borrowTime);

            //填充借阅记录
            strcpy(records[recordCount].bookId, books[i].id);
            strcpy(records[recordCount].readerId, curUser.id);
            strcpy(records[recordCount].borrowTime, borrowTime);
            strcpy(records[recordCount].returnTime, "未归还");
            records[recordCount].isReturned = 0;
            recordCount++;

            // 将借阅号添加到读者结构体的数组中
            int readerIdx = -1;
            for (int k = 0; k < readerCount; k++) {
                if (strcmp(readers[k].id, curUser.id) == 0) {
                    readerIdx = k;
                    break;
                }
            }
            if (readerIdx != -1) {
                strcpy(readers[readerIdx].borrowIds[readers[readerIdx].borrowIdCount], borrowId);
                readers[readerIdx].borrowIdCount++;
                // 同步更新当前用户的借阅号信息
                strcpy(curUser.borrowIds[curUser.borrowIdCount], borrowId);
                curUser.borrowIdCount++;
            }

            //更新图书和读者状态
            books[i].stock--;
            books[i].borrowCount++;
            for (int j = 0; j < readerCount; j++) {
                if (strcmp(readers[j].id, curUser.id) == 0) {
                    readers[j].borrowed++;
                    break;
                }
            }

            SetPosition(POS_X2, ++posy);
            printf("借阅成功！生成借阅号：%s\n", borrowId);
            return;
        }
    }

    if (!flag) {
        SetPosition(POS_X2, ++posy);
        printf("未找到该图书！\n");
    }
}

//图书归还（从读者结构体中移除借阅号）
void ReturnBook(READER curUser) {
    char bookId[ID_LEN], returnTime[20];
    char borrowId[BORROW_ID_LEN]; // 临时存储匹配的借阅号
    int i, j, k, flag = 0, posy = 6;
    SetPosition(POS_X2, posy);
    printf("输入要归还的图书编号：");
    scanf("%s", bookId);
    SetPosition(POS_X2, ++posy);
    printf("输入归还时间（YYYYMMDD）：");
    scanf("%s", returnTime);

    for (i = 0; i < bookCount; i++) {
        if (strcmp(books[i].id, bookId) == 0) {
            flag = 1;
            //更新图书库存
            books[i].stock++;
            //更新读者借阅数量
            for (j = 0; j < readerCount; j++) {
                if (strcmp(readers[j].id, curUser.id) == 0) {
                    readers[j].borrowed--;
                    break;
                }
            }
            //更新借阅记录并移除读者的借阅号
            for (j = 0; j < recordCount; j++) {
                if (strcmp(records[j].bookId, bookId) == 0 &&
                    strcmp(records[j].readerId, curUser.id) == 0 &&
                    records[j].isReturned == 0) {
                    strcpy(records[j].returnTime, returnTime);
                    records[j].isReturned = 1;

                    // 生成对应借阅号（用于匹配删除）
                    GenerateBorrowId(borrowId, records[j].bookId, records[j].readerId, records[j].borrowTime);

                    // 从读者结构体中删除该借阅号
                    int readerIdx = -1;
                    for (k = 0; k < readerCount; k++) {
                        if (strcmp(readers[k].id, curUser.id) == 0) {
                            readerIdx = k;
                            break;
                        }
                    }
                    if (readerIdx != -1) {
                        for (k = 0; k < readers[readerIdx].borrowIdCount; k++) {
                            if (strcmp(readers[readerIdx].borrowIds[k], borrowId) == 0) {
                                // 数组前移覆盖
                                for (int m = k; m < readers[readerIdx].borrowIdCount - 1; m++) {
                                    strcpy(readers[readerIdx].borrowIds[m], readers[readerIdx].borrowIds[m+1]);
                                }
                                // 清空最后一个元素
                                memset(readers[readerIdx].borrowIds[readers[readerIdx].borrowIdCount - 1], 0, BORROW_ID_LEN);
                                readers[readerIdx].borrowIdCount--;
                                // 同步更新当前用户
                                curUser.borrowIdCount--;
                                break;
                            }
                        }
                    }

                    break;
                }
            }
            SetPosition(POS_X2, ++posy);
            printf("归还成功！\n");
            return;
        }
    }

    if (!flag) {
        SetPosition(POS_X2, ++posy);
        printf("未找到该图书！\n");
    }
}

//按图书/读者编号查询借阅记录
void QueryBorrowRecord(int n, BORROW_RECORD record[]) {
    char key[ID_LEN];
    int choice, i, flag = 0, posy = 6;
    SetPosition(POS_X2, posy);
    printf("1. 按图书编号查询  2. 按读者编号查询\n");
    SetPosition(POS_X2, ++posy);
    printf("选择查询方式：");
    scanf("%d", &choice);
    SetPosition(POS_X2, ++posy);
    printf("输入查询关键词：");
    scanf("%s", key);

    SetPosition(POS_X1, ++posy);
    printf("===== 借阅记录查询结果 =====\n");
    if (choice == 1) {        // 按图书编号查询
        for (i = 0; i < n; i++) {
            if (strcmp(record[i].bookId, key) == 0) {
                flag = 1;
                SetPosition(POS_X2, ++posy);
                printf("图书编号：%s  读者编号：%s\n", record[i].bookId, record[i].readerId);
                printf("借阅时间：%s  归还时间：%s  状态：%s\n",
                       record[i].borrowTime, record[i].returnTime,
                       record[i].isReturned ? "已归还" : "未归还");
                posy += 2;
            }
        }
    } else if (choice == 2) {
        // 按读者编号查询
        for (i = 0; i < n; i++) {
            if (strcmp(record[i].readerId, key) == 0) {
                flag = 1;
                SetPosition(POS_X2, ++posy);
                printf("读者编号：%s  图书编号：%s\n", record[i].readerId, record[i].bookId);
                printf("借阅时间：%s  归还时间：%s  状态：%s\n",
                       record[i].borrowTime, record[i].returnTime,
                       record[i].isReturned ? "已归还" : "未归还");
                posy += 2;
            }
        }
    } else {
        SetPosition(POS_X2, ++posy);
        printf("查询方式输入错误！\n");
        return;
    }

    if (!flag) {
        SetPosition(POS_X2, ++posy);
        printf("未查询到相关借阅记录！\n");
    }
}

// 按作者查询借阅记录
void QueryBorrowByAuthor(int n, BORROW_RECORD record[], BOOK books[]) {
    char author[NAME_LEN];
    int i, j, flag = 0, posy = 6;
    SetPosition(POS_X2, posy);
    printf("输入要查询的作者姓名：");
    scanf("%s", author);

    SetPosition(POS_X1, ++posy);
    printf("===== 作者 %s 相关借阅记录 =====\n", author);
    for (i = 0; i < n; i++) {
        // 匹配图书编号对应的作者
        for (j = 0; j < bookCount; j++) {
            if (strcmp(record[i].bookId, books[j].id) == 0 && strcmp(books[j].author, author) == 0) {
                flag = 1;
                SetPosition(POS_X2, ++posy);
                printf("图书编号：%s  书名：%s  读者编号：%s\n", books[j].id, books[j].name, record[i].readerId);
                printf("借阅时间：%s  归还时间：%s  状态：%s\n",
                       record[i].borrowTime, record[i].returnTime,
                       record[i].isReturned ? "已归还" : "未归还");
                posy += 2;
                break;
            }
        }
    }

    if (!flag) {
        SetPosition(POS_X2, ++posy);
        printf("未查询到该作者的相关借阅记录！\n");
    }
}

// 按图书名称/作者查询图书
void SearchBookByNameOrAuthor(int n, BOOK book[]) {
    char keyword[NAME_LEN];
    int choice, i, flag = 0, posy = 6;
    system("cls");
    SetPosition(POS_X2, posy);
    printf("1. 按书名查询  2. 按作者查询\n");
    SetPosition(POS_X2, ++posy);
    printf("选择查询方式：");
    scanf("%d", &choice);
    SetPosition(POS_X2, ++posy);
    printf("输入查询关键词：");
    scanf("%s", keyword);

    SetPosition(POS_X1, ++posy);
    printf("===== 图书查询结果 =====\n");
    if (choice == 1) {
        // 按书名查询
        for (i = 0; i < n; i++) {
            if (strstr(book[i].name, keyword) != NULL) {
                flag = 1;
                SetPosition(POS_X2, ++posy);
                printf("编号：%s  书名：%s  作者：%s  库存：%d  价格：%.2f\n",
                       book[i].id, book[i].name, book[i].author, book[i].stock, book[i].price);
                posy++;
            }
        }
    } else if (choice == 2) {
        // 按作者查询
        for (i = 0; i < n; i++) {
            if (strstr(book[i].author, keyword) != NULL) {
                flag = 1;
                SetPosition(POS_X2, ++posy);
                printf("编号：%s  书名：%s  作者：%s  库存：%d  价格：%.2f\n",
                       book[i].id, book[i].name, book[i].author, book[i].stock, book[i].price);
                posy++;
            }
        }
    } else {
        SetPosition(POS_X2, ++posy);
        printf("查询方式输入错误！\n");
        return;
    }

    if (!flag) {
        SetPosition(POS_X2, ++posy);
        printf("未查询到相关图书！\n");
    }
}

// 查询个人借阅记录
void QueryMyRecord(char *readerId) {
    int i, flag = 0, posy = 6;
    SetPosition(POS_X2, posy);
    printf("===== 个人借阅记录 =====\n");
    for (i = 0; i < recordCount; i++) {
        if (strcmp(records[i].readerId, readerId) == 0) {
            flag = 1;
            SetPosition(POS_X2, ++posy);
            printf("图书编号：%s  借阅时间：%s  归还时间：%s  状态：%s\n",
                   records[i].bookId, records[i].borrowTime, records[i].returnTime,
                   records[i].isReturned ? "已归还" : "未归还");
            posy++;
        }
    }

    if (!flag) {
        SetPosition(POS_X2, ++posy);
        printf("暂无个人借阅记录！\n");
    }
}

// 借阅次数TOP10统计
void StatBorrowTop10(int n, BOOK book[]) {
    int i, j, maxIdx;
    BOOK temp;
    // 临时拷贝数组，避免打乱原数据
    BOOK tempBooks[BOOK_NUM];
    for (i = 0; i < n; i++) {
        tempBooks[i] = book[i];
    }

    // 冒泡排序（降序，按借阅次数）
    for (i = 0; i < n - 1; i++) {
        maxIdx = i;
        for (j = i + 1; j < n; j++) {
            if (tempBooks[j].borrowCount > tempBooks[maxIdx].borrowCount) {
                maxIdx = j;
            }
        }
        temp = tempBooks[i];
        tempBooks[i] = tempBooks[maxIdx];
        tempBooks[maxIdx] = temp;
    }

    // 输出TOP10
    SetPosition(POS_X2, 5);
    printf("===== 图书借阅次数TOP10 =====\n");
    int count = n > 10 ? 10 : n;
    for (i = 0; i < count; i++) {
        SetPosition(POS_X2, 6 + i);
        printf("第%d名：%s 《%s》  借阅次数：%d\n",
               i + 1, tempBooks[i].id, tempBooks[i].name, tempBooks[i].borrowCount);
    }
    if (n == 0) {
        SetPosition(POS_X2, 6);
        printf("暂无图书借阅数据！\n");
    }
}

// 打印所有图书信息
void PrintBook(int n, BOOK book[]) {
    int i, posy = 5;
    SetPosition(POS_X2, posy);
    printf("===== 所有图书信息 =====\n");
    for (i = 0; i < n; i++) {
        SetPosition(POS_X2, ++posy);
        printf("编号：%s  书名：%s  作者：%s  分类号：%s\n",
               book[i].id, book[i].name, book[i].author, book[i].category);
        SetPosition(POS_X2, ++posy);
        printf("出版社：%s  出版时间：%s  库存：%d  价格：%.2f  借阅次数：%d\n",
               book[i].publisher, book[i].publishTime, book[i].stock, book[i].price, book[i].borrowCount);
        posy++;
    }
    if (n == 0) {
        SetPosition(POS_X2, 6);
        printf("暂无图书信息！\n");
    }
}

// 打印所有读者信息
void PrintReader(int n, READER reader[]) {
    int i, posy = 5;
    SetPosition(POS_X2, posy);
    printf("===== 所有读者信息 =====\n");
    for (i = 0; i < n; i++) {
        SetPosition(POS_X2, ++posy);
        printf("编号：%s  姓名：%s  最大借阅量：%d  已借阅：%d  身份：%s\n",
               reader[i].id, reader[i].name, reader[i].maxBorrow, reader[i].borrowed,
               reader[i].isAdmin ? "管理员" : "普通读者");
        posy++;
    }
    if (n == 0) {
        SetPosition(POS_X2, 6);
        printf("暂无读者信息！\n");
    }
}

// 查看个人信息（含借阅号）
void ShowPersonalInfo(READER user) {
  // ========== 新增：同步所有用户信息（覆盖副本旧数据） ==========
    int readerIdx = -1;
    // 1. 从全局readers数组找到当前用户
    for (int i = 0; i < readerCount; i++) {
        if (strcmp(readers[i].id, user.id) == 0) {
            readerIdx = i;
            break;
        }
    }
    // 2. 同步所有字段：姓名、已借阅数、借阅号数组、借阅号数量
    if (readerIdx != -1) {
        strcpy(user.name, readers[readerIdx].name);          // 同步姓名
        user.borrowed = readers[readerIdx].borrowed;          // 同步已借阅数量
        memcpy(user.borrowIds, readers[readerIdx].borrowIds, sizeof(user.borrowIds)); // 同步借阅号数组
        user.borrowIdCount = readers[readerIdx].borrowIdCount;// 同步借阅号数量
    }
    // ==========================================

    int posy = 5;
    SetPosition(POS_X2, posy);
    printf("===== 个人信息 =====\n");
    SetPosition(POS_X2, ++posy);
    printf("读者编号：%s\n", user.id);
    SetPosition(POS_X2, ++posy);
    printf("姓名：%s\n", user.name);
    SetPosition(POS_X2, ++posy);
    printf("最大借阅量：%d\n", user.maxBorrow);
    SetPosition(POS_X2, ++posy);
    printf("已借阅数量：%d\n", user.borrowed);
    SetPosition(POS_X2, ++posy);
    printf("当前借阅号列表：\n");
    if (user.borrowIdCount == 0) {
        SetPosition(POS_X2, ++posy);
        printf("  无未归还借阅记录\n");
    } else {
        for (int i = 0; i < user.borrowIdCount; i++) {
            SetPosition(POS_X2, ++posy);
            printf("  %s\n", user.borrowIds[i]);
        }
    }
}

// 数据存盘函数
void WritetoFile() {
    FILE *fp;
    // 保存图书数据
    fp = fopen("books.dat", "wb");
    if (fp == NULL) {
        printf("图书数据文件打开失败！\n");
        return;
    }
    fwrite(&bookCount, sizeof(int), 1, fp);
    fwrite(books, sizeof(BOOK), bookCount, fp);
    fclose(fp);

    // 保存读者数据
    fp = fopen("readers.dat", "wb");
    if (fp == NULL) {
        printf("读者数据文件打开失败！\n");
        return;
    }
    fwrite(&readerCount, sizeof(int), 1, fp);
    fwrite(readers, sizeof(READER), readerCount, fp);
    fclose(fp);

    // 保存借阅记录数据
    fp = fopen("records.dat", "wb");
    if (fp == NULL) {
        printf("借阅记录文件打开失败！\n");
        return;
    }
    fwrite(&recordCount, sizeof(int), 1, fp);
    fwrite(records, sizeof(BORROW_RECORD), recordCount, fp);
    fclose(fp);
}

// 从文件读取数据函数（修复返回值bug）
int ReadfromFile() {
    FILE *fp;
    // 读取图书数据
    fp = fopen("books.dat", "rb");
    if (fp == NULL) {
        return 1; // 无文件返回1
    }
    fread(&bookCount, sizeof(int), 1, fp);
    fread(books, sizeof(BOOK), bookCount, fp);
    fclose(fp);

    // 读取读者数据
    fp = fopen("readers.dat", "rb");
    if (fp == NULL) {
        return 1;
    }
    fread(&readerCount, sizeof(int), 1, fp);
    fread(readers, sizeof(READER), readerCount, fp);
    fclose(fp);

    // 读取借阅记录数据
    fp = fopen("records.dat", "rb");
    if (fp == NULL) {
        return 1;
    }
    fread(&recordCount, sizeof(int), 1, fp);
    fread(records, sizeof(BORROW_RECORD), recordCount, fp);
    fclose(fp);

    return 0; // 读取成功返回0
}