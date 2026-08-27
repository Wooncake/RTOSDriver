# LED驱动说明

#### **Driver层** （只负责逻辑描述，功能的实现）

**Led_Init(led_handle_t *handle, led_io_t *io, uint8_t active_level)** （连接HAL或其他的桥梁）

**drv_status_t LED_Set(led_handle_t *handle, bool on);**

**drv_status_t LED_On(led_handle_t *handle);**

**drv_status_t LED_Off(led_handle_t *handle);**

**drv_status_t LED_Toggle(led_handle_t *handle);**

**bool LED_IsOn(const led_handle_t *handle);**

五个功能函数，只需调用LED句柄即可使用



### BSP层

只做实际IO口的初始化，连接具体的设备



### APP层

只负责实际功能的运行



## 每次写一个外设前，先回答下面几个问题

**这个模块提供什么功能？**

**哪些东西是所有芯片都一样的？**

**哪些东西只属于某个芯片或某块板？**

**这个模块有哪些状态？**

**上层真正需要什么接口？**

**这个模块是同步还是异步？**

**是否允许多个实例？**

**发生错误时如何通知上层？**

**是否需要周期调用 `Update()`？**

**如何在没有真实硬件时测试？**

![image-20260825154310302](C:\Users\wjq13\AppData\Roaming\Typora\typora-user-images\image-20260825154310302.png)

# 嵌入式代码最重要的是：

## 接口清楚

## 状态明确

## 依赖可控

## 行为可测

## 资源占用可预测

## 出错时可诊断



## 应用层：只表达业务

## 驱动层：只实现设备行为

## BSP 层：只连接具体硬件

## 芯片层：只处理具体 HAL/寄存器



