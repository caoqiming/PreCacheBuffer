
#ifndef CONSTANT_H_
#define CONSTANT_H_

const size_t MAX_BUFFER_SIZE = 1073741824; //byte,1Gb 如果config.json里没有定义最大缓存则就用这个
const int StrategyByTime = 1; 
const int StrategyByCFAE = 2; //基于自编码器的协同过滤
#endif // CONSTANT_H_

