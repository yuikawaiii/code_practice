#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <ctype.h> // 为了使用 toupper() 函数

int main() {
    char playAgain;

    // 外层循环，控制是否继续游戏
    do {
        int target, guess, count = 0;
        
        // 为每一局游戏生成新的随机数种子（虽然放在这里和外面效果差不多，但逻辑更清晰）
        // 注：实际上 srand() 只需要在程序开始时调用一次即可。
        srand((unsigned int)time(NULL));
        target = rand() % 10 + 1;

        printf("==================== 猜数字游戏 ====================\n");
        printf("规则：系统已生成 1~10 的随机数，猜中即可获胜！\n");
        printf("每次猜测后会提示“猜大了”或“猜小了”。\n");
        printf("===================================================\n\n");

        // 内层循环，控制一局游戏的猜测过程
        while (1) {
            printf("请输入猜测的数字（1-10）：");
            scanf("%d", &guess);
            count++;

            if (guess < 1 || guess > 10) {
                printf("输入无效！请输入 1-10 之间的数字。\n\n");
                continue;
            }

            if (guess > target) {
                printf("猜大了！再试试？\n\n");
            } else if (guess < target) {
                printf("猜小了！再努力一下？\n\n");
            } else {
                break; // 猜对了，跳出内层循环
            }
        }

        printf("\n? 恭喜你，猜对了！目标数字是 %d ！\n", target);
        printf("你一共猜了 %d 次。\n", count);

        // 询问玩家是否继续
        printf("\n是否要再来一局？(Y/N): ");
        // 注意：在 scanf 之前最好清空缓冲区，避免上次输入的换行符影响
        while (getchar() != '\n'); // 清空缓冲区
        scanf("%c", &playAgain);

    } while (toupper(playAgain) == 'Y'); // 如果玩家输入 Y 或 y，则继续循环

    printf("\n感谢游玩！再见！\n");
    
    // 程序正常结束
    return 0;
}