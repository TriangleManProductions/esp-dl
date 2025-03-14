如何部署 YOLO11n
====================

:link_to_translation:`en:[English]`

在本教程中，我们介绍如何使用 ESP-PPQ 对预训练的 YOLO11n 模型进行量化，并使用 ESP-DL 部署量化后的 YOLO11n 模型。

.. contents::
  :local:
  :depth: 2

准备工作
--------

1. :ref:`安装 ESP_IDF <requirements_esp_idf>`
2. :ref:`安装 ESP_PPQ <requirements_esp_ppq>`

.. _how_to_quantize_yolo11n:

模型量化
--------

预训练模型
^^^^^^^^^^^^

你可以从 `Ultralytics release <https://github.com/ultralytics/assets/releases/download/v8.3.0/yolo11n.pt>`__ 下载预训练的 yolo11n 模型。

目前ESP-PPQ支持 ONNX、PyTorch、TensorFlow 模型。在量化过程中，PyTorch 和 TensorFlow 会先转化为 ONNX 模型，因此将与训练的 yolo11n 转化成ONNX模型。

具体来说，参考脚本： :project_file:`export_onnx.py <models/coco_detect/models/export_onnx.py>` 将预训练的 yolo11n 模型转换为 ONNX 模型。

在该脚本中，我们重载了 Detect 类的 forward 方法，具有以下优势：

- 更快的推理速度。 与原始的 yolo11n 模型相比, 将推理过程中 Detect 里与解码边界框相关的操作移至后处理中完成, 从而显著减少了推理延迟。一方面，``Conv``， ``Transpose``， ``Slice``， ``Split`` 和 ``Concat`` 操作在推理过程中运行是非常耗时的。另一方面，在后处理阶段，模型推理的输出首先进行置信度筛选，然后再解码边界框，这大大减少了计算量，从而加快了整体推理速度。

- 更低的量化误差。 ESP-PPQ中的 ``Concat`` 和 ``Add`` 操作采用了联合量化。为了减少量化误差，由于 box 和 score 的范围差异较大，它们通过不同的分支输出，而不是拼接在一起。类似地，由于 ``Add`` 和 ``Sub`` 的输入的范围差异较大，相关计算被移到了后处理中进行，避免被量化。


校准数据集
^^^^^^^^^^^^

校准数据集需要和模型输入格式一致，同时尽可能覆盖模型输入的所有可能情况，以便更好地量化模型。本示例中，我们使用的校准集为 `calib_yolo11n <https://dl.espressif.com/public/calib_yolo11n.zip>`__ 。

8bit 默认配置量化
^^^^^^^^^^^^^^^^^^^

**量化设置**

.. code-block:: python

   target="esp32p4"
   num_of_bits=8
   batch_size=32
   quant_setting = QuantizationSettingFactory.espdl_setting() # default setting

**量化结果**

.. code-block::

   Layer                                        | NOISE:SIGNAL POWER RATIO 
   /model.10/m/m.0/ffn/ffn.1/conv/Conv:         | ████████████████████ | 36.163%
   /model.10/m/m.0/attn/proj/conv/Conv:         | ████████████████     | 28.768%
   /model.23/cv3.2/cv3.2.0/cv3.2.0.0/conv/Conv: | █████████████        | 22.992%
   /model.23/cv2.2/cv2.2.0/conv/Conv:           | ████████████         | 21.672%
   /model.23/cv3.2/cv3.2.1/cv3.2.1.1/conv/Conv: | ████████████         | 21.452%
   /model.23/cv2.2/cv2.2.1/conv/Conv:           | ████████████         | 21.120%
   /model.23/cv3.2/cv3.2.0/cv3.2.0.1/conv/Conv: | ████████████         | 21.048%
   /model.23/cv3.1/cv3.1.1/cv3.1.1.1/conv/Conv: | ███████████          | 19.348%
   /model.22/m.0/cv2/conv/Conv:                 | ███████████          | 19.318%
   /model.23/cv3.0/cv3.0.1/cv3.0.1.1/conv/Conv: | ███████████          | 19.243%
   /model.22/m.0/cv3/conv/Conv:                 | ███████████          | 19.083%
   /model.22/cv2/conv/Conv:                     | ██████████           | 18.588%
   /model.22/m.0/m/m.1/cv2/conv/Conv:           | ██████████           | 18.305%
   /model.23/cv2.1/cv2.1.1/conv/Conv:           | ██████████           | 17.486%
   /model.8/m.0/cv2/conv/Conv:                  | █████████            | 16.203%
   /model.23/cv2.0/cv2.0.1/conv/Conv:           | █████████            | 15.642%
   /model.10/m/m.0/attn/pe/conv/Conv:           | ████████             | 14.710%
   /model.10/m/m.0/attn/qkv/conv/Conv:          | ████████             | 14.621%
   /model.23/cv2.1/cv2.1.0/conv/Conv:           | ████████             | 14.169%
   /model.22/cv1/conv/Conv:                     | ████████             | 14.113%
   /model.10/m/m.0/attn/MatMul_1:               | ████████             | 14.007%
   /model.10/cv1/conv/Conv:                     | ███████              | 13.563%
   /model.23/cv3.1/cv3.1.0/cv3.1.0.1/conv/Conv: | ██████               | 11.779%
   /model.19/m.0/cv2/conv/Conv:                 | ██████               | 11.213%
   /model.22/m.0/m/m.0/cv2/conv/Conv:           | ██████               | 11.177%
   /model.13/m.0/cv2/conv/Conv:                 | ██████               | 10.887%
   /model.23/cv3.2/cv3.2.1/cv3.2.1.0/conv/Conv: | ██████               | 10.829%
   /model.20/conv/Conv:                         | ██████               | 10.704%
   /model.23/cv2.2/cv2.2.2/Conv:                | █████                | 9.922%
   /model.10/cv2/conv/Conv:                     | █████                | 9.783%
   /model.8/cv2/conv/Conv:                      | █████                | 9.493%
   /model.8/m.0/cv1/conv/Conv:                  | █████                | 9.428%
   /model.19/cv2/conv/Conv:                     | █████                | 9.104%
   /model.8/cv1/conv/Conv:                      | █████                | 9.095%
   /model.8/m.0/cv3/conv/Conv:                  | █████                | 9.091%
   /model.22/m.0/m/m.0/cv1/conv/Conv:           | █████                | 9.035%
   /model.23/cv3.0/cv3.0.0/cv3.0.0.1/conv/Conv: | █████                | 9.015%
   /model.6/m.0/cv2/conv/Conv:                  | █████                | 8.890%
   /model.22/m.0/m/m.1/cv1/conv/Conv:           | █████                | 8.676%
   /model.13/cv2/conv/Conv:                     | █████                | 8.543%
   /model.8/m.0/m/m.0/cv1/conv/Conv:            | █████                | 8.472%
   /model.8/m.0/m/m.0/cv2/conv/Conv:            | █████                | 8.413%
   /model.19/cv1/conv/Conv:                     | ████                 | 8.194%
   /model.8/m.0/m/m.1/cv1/conv/Conv:            | ████                 | 8.050%
   /model.13/cv1/conv/Conv:                     | ████                 | 7.896%
   /model.10/m/m.0/attn/MatMul:                 | ████                 | 7.890%
   /model.19/m.0/cv1/conv/Conv:                 | ████                 | 7.540%
   /model.22/m.0/cv1/conv/Conv:                 | ████                 | 7.249%
   /model.8/m.0/m/m.1/cv2/conv/Conv:            | ████                 | 7.049%
   /model.23/cv2.0/cv2.0.0/conv/Conv:           | ████                 | 7.037%
   /model.13/m.0/cv1/conv/Conv:                 | ████                 | 6.964%
   /model.23/cv2.1/cv2.1.2/Conv:                | ████                 | 6.782%
   /model.23/cv2.0/cv2.0.2/Conv:                | ████                 | 6.773%
   /model.23/cv3.1/cv3.1.1/cv3.1.1.0/conv/Conv: | ████                 | 6.644%
   /model.17/conv/Conv:                         | ███                  | 6.031%
   /model.16/m.0/cv2/conv/Conv:                 | ███                  | 5.914%
   /model.6/m.0/cv3/conv/Conv:                  | ███                  | 5.824%
   /model.6/cv2/conv/Conv:                      | ███                  | 5.817%
   /model.6/cv1/conv/Conv:                      | ███                  | 5.687%
   /model.7/conv/Conv:                          | ███                  | 5.572%
   /model.9/cv2/conv/Conv:                      | ███                  | 5.381%
   /model.10/m/m.0/ffn/ffn.0/conv/Conv:         | ███                  | 5.195%
   /model.6/m.0/m/m.0/cv1/conv/Conv:            | ███                  | 5.167%
   /model.16/m.0/cv1/conv/Conv:                 | ███                  | 5.112%
   /model.16/cv2/conv/Conv:                     | ███                  | 4.991%
   /model.23/cv3.1/cv3.1.0/cv3.1.0.0/conv/Conv: | ███                  | 4.978%
   /model.2/cv2/conv/Conv:                      | ██                   | 4.553%
   /model.6/m.0/m/m.0/cv2/conv/Conv:            | ██                   | 4.448%
   /model.3/conv/Conv:                          | ██                   | 4.320%
   /model.23/cv3.0/cv3.0.1/cv3.0.1.0/conv/Conv: | ██                   | 4.317%
   /model.6/m.0/m/m.1/cv1/conv/Conv:            | ██                   | 3.968%
   /model.5/conv/Conv:                          | ██                   | 3.947%
   /model.6/m.0/cv1/conv/Conv:                  | ██                   | 3.865%
   /model.4/cv1/conv/Conv:                      | ██                   | 3.720%
   /model.2/cv1/conv/Conv:                      | ██                   | 3.565%
   /model.4/cv2/conv/Conv:                      | ██                   | 3.542%
   /model.16/cv1/conv/Conv:                     | ██                   | 3.121%
   /model.2/m.0/cv2/conv/Conv:                  | █                    | 2.843%
   /model.6/m.0/m/m.1/cv2/conv/Conv:            | █                    | 2.767%
   /model.4/m.0/cv1/conv/Conv:                  | █                    | 2.533%
   /model.9/cv1/conv/Conv:                      | █                    | 2.017%
   /model.4/m.0/cv2/conv/Conv:                  | █                    | 1.761%
   /model.23/cv3.0/cv3.0.0/cv3.0.0.0/conv/Conv: | █                    | 1.324%
   /model.1/conv/Conv:                          | █                    | 1.315%
   /model.23/cv3.2/cv3.2.2/Conv:                | █                    | 1.113%
   /model.2/m.0/cv1/conv/Conv:                  |                      | 0.731%
   /model.23/cv3.1/cv3.1.2/Conv:                |                      | 0.491%
   /model.23/cv3.0/cv3.0.2/Conv:                |                      | 0.282%
   /model.0/conv/Conv:                          |                      | 0.159%
   Analysing Layerwise quantization error:: 100%|██████████| 89/89 [04:30<00:00,  3.04s/it]
   Layer                                        | NOISE:SIGNAL POWER RATIO 
   /model.1/conv/Conv:                          | ████████████████████ | 0.384%
   /model.22/cv1/conv/Conv:                     | █████████████        | 0.247%
   /model.4/cv2/conv/Conv:                      | ████████████         | 0.233%
   /model.2/cv2/conv/Conv:                      | ██████████           | 0.201%
   /model.0/conv/Conv:                          | ██████████           | 0.192%
   /model.9/cv2/conv/Conv:                      | ████████             | 0.156%
   /model.10/cv1/conv/Conv:                     | ███████              | 0.132%
   /model.3/conv/Conv:                          | ██████               | 0.108%
   /model.4/cv1/conv/Conv:                      | ████                 | 0.074%
   /model.16/cv1/conv/Conv:                     | ███                  | 0.066%
   /model.2/cv1/conv/Conv:                      | ███                  | 0.060%
   /model.23/cv2.0/cv2.0.0/conv/Conv:           | ███                  | 0.052%
   /model.2/m.0/cv1/conv/Conv:                  | ██                   | 0.044%
   /model.6/cv1/conv/Conv:                      | ██                   | 0.033%
   /model.10/m/m.0/attn/pe/conv/Conv:           | ██                   | 0.029%
   /model.2/m.0/cv2/conv/Conv:                  | █                    | 0.028%
   /model.22/m.0/m/m.0/cv1/conv/Conv:           | █                    | 0.023%
   /model.16/cv2/conv/Conv:                     | █                    | 0.021%
   /model.16/m.0/cv2/conv/Conv:                 | █                    | 0.020%
   /model.19/m.0/cv1/conv/Conv:                 | █                    | 0.020%
   /model.4/m.0/cv1/conv/Conv:                  | █                    | 0.018%
   /model.19/cv2/conv/Conv:                     | █                    | 0.017%
   /model.4/m.0/cv2/conv/Conv:                  | █                    | 0.016%
   /model.10/m/m.0/attn/qkv/conv/Conv:          | █                    | 0.016%
   /model.19/cv1/conv/Conv:                     | █                    | 0.015%
   /model.13/cv2/conv/Conv:                     | █                    | 0.015%
   /model.8/cv1/conv/Conv:                      | █                    | 0.013%
   /model.23/cv2.1/cv2.1.0/conv/Conv:           | █                    | 0.013%
   /model.23/cv2.2/cv2.2.1/conv/Conv:           | █                    | 0.012%
   /model.13/cv1/conv/Conv:                     | █                    | 0.012%
   /model.10/cv2/conv/Conv:                     | █                    | 0.011%
   /model.13/m.0/cv1/conv/Conv:                 | █                    | 0.011%
   /model.6/cv2/conv/Conv:                      | █                    | 0.011%
   /model.13/m.0/cv2/conv/Conv:                 | █                    | 0.010%
   /model.5/conv/Conv:                          |                      | 0.010%
   /model.19/m.0/cv2/conv/Conv:                 |                      | 0.009%
   /model.6/m.0/m/m.1/cv1/conv/Conv:            |                      | 0.009%
   /model.23/cv3.0/cv3.0.0/cv3.0.0.1/conv/Conv: |                      | 0.008%
   /model.23/cv2.2/cv2.2.0/conv/Conv:           |                      | 0.008%
   /model.23/cv2.1/cv2.1.1/conv/Conv:           |                      | 0.008%
   /model.9/cv1/conv/Conv:                      |                      | 0.008%
   /model.23/cv2.0/cv2.0.1/conv/Conv:           |                      | 0.007%
   /model.16/m.0/cv1/conv/Conv:                 |                      | 0.007%
   /model.17/conv/Conv:                         |                      | 0.007%
   /model.23/cv3.1/cv3.1.1/cv3.1.1.0/conv/Conv: |                      | 0.007%
   /model.10/m/m.0/ffn/ffn.1/conv/Conv:         |                      | 0.007%
   /model.23/cv2.0/cv2.0.2/Conv:                |                      | 0.006%
   /model.8/m.0/cv1/conv/Conv:                  |                      | 0.006%
   /model.23/cv2.2/cv2.2.2/Conv:                |                      | 0.005%
   /model.23/cv2.1/cv2.1.2/Conv:                |                      | 0.005%
   /model.22/m.0/cv3/conv/Conv:                 |                      | 0.005%
   /model.23/cv3.1/cv3.1.0/cv3.1.0.1/conv/Conv: |                      | 0.005%
   /model.7/conv/Conv:                          |                      | 0.005%
   /model.8/cv2/conv/Conv:                      |                      | 0.004%
   /model.22/cv2/conv/Conv:                     |                      | 0.004%
   /model.6/m.0/cv3/conv/Conv:                  |                      | 0.004%
   /model.10/m/m.0/ffn/ffn.0/conv/Conv:         |                      | 0.004%
   /model.8/m.0/m/m.1/cv2/conv/Conv:            |                      | 0.004%
   /model.22/m.0/m/m.1/cv1/conv/Conv:           |                      | 0.004%
   /model.8/m.0/m/m.1/cv1/conv/Conv:            |                      | 0.004%
   /model.23/cv3.1/cv3.1.1/cv3.1.1.1/conv/Conv: |                      | 0.003%
   /model.10/m/m.0/attn/proj/conv/Conv:         |                      | 0.003%
   /model.22/m.0/m/m.0/cv2/conv/Conv:           |                      | 0.003%
   /model.22/m.0/cv1/conv/Conv:                 |                      | 0.003%
   /model.8/m.0/cv3/conv/Conv:                  |                      | 0.003%
   /model.6/m.0/m/m.0/cv1/conv/Conv:            |                      | 0.003%
   /model.23/cv3.0/cv3.0.0/cv3.0.0.0/conv/Conv: |                      | 0.003%
   /model.23/cv3.2/cv3.2.1/cv3.2.1.0/conv/Conv: |                      | 0.002%
   /model.6/m.0/m/m.1/cv2/conv/Conv:            |                      | 0.002%
   /model.8/m.0/m/m.0/cv2/conv/Conv:            |                      | 0.002%
   /model.23/cv3.2/cv3.2.1/cv3.2.1.1/conv/Conv: |                      | 0.002%
   /model.10/m/m.0/attn/MatMul_1:               |                      | 0.002%
   /model.22/m.0/m/m.1/cv2/conv/Conv:           |                      | 0.001%
   /model.6/m.0/m/m.0/cv2/conv/Conv:            |                      | 0.001%
   /model.23/cv3.0/cv3.0.1/cv3.0.1.0/conv/Conv: |                      | 0.001%
   /model.8/m.0/m/m.0/cv1/conv/Conv:            |                      | 0.001%
   /model.23/cv3.2/cv3.2.0/cv3.2.0.1/conv/Conv: |                      | 0.001%
   /model.23/cv3.0/cv3.0.1/cv3.0.1.1/conv/Conv: |                      | 0.001%
   /model.6/m.0/cv1/conv/Conv:                  |                      | 0.001%
   /model.23/cv3.2/cv3.2.2/Conv:                |                      | 0.001%
   /model.20/conv/Conv:                         |                      | 0.001%
   /model.23/cv3.1/cv3.1.2/Conv:                |                      | 0.001%
   /model.23/cv3.2/cv3.2.0/cv3.2.0.0/conv/Conv: |                      | 0.001%
   /model.6/m.0/cv2/conv/Conv:                  |                      | 0.001%
   /model.23/cv3.0/cv3.0.2/Conv:                |                      | 0.000%
   /model.10/m/m.0/attn/MatMul:                 |                      | 0.000%
   /model.23/cv3.1/cv3.1.0/cv3.1.0.0/conv/Conv: |                      | 0.000%
   /model.8/m.0/cv2/conv/Conv:                  |                      | 0.000%
   /model.22/m.0/cv2/conv/Conv:                 |                      | 0.000%

**量化误差分析**

在相同输入下，量化后的模型在 COCO val2017 上的 mAP50:95 仅为 30.8%，低于浮点模型，存在一定的精度损失：

- **累计误差 (Graphwise Error)**

   模型的输出层是 /model.23/cv3.2/cv3.2.2/Conv，/model.23/cv2.2/cv2.2.2/Conv，/model.23/cv3.1/cv3.1.2/Conv，/model.23/cv2.1/cv2.1.2/Conv，/model.23/cv3.0/cv3.0.2/Conv 和 /model.23/cv2.0/cv2.0.2/Conv，累计误差分别为 1.113%，9.922%，0.491%，6.782%，0.282% 和 6.773% 。通常，如果输出层的累计误差小于 10%，则量化模型的精度损失较小。

- **逐层误差 (Layerwise error)**

   观察逐层误差发现，所有层的误差均低于 1%，这表明所有层的量化误差都很小。

我们注意到，虽然所有层的逐层误差都很小，但是一些层的累计误差却较大。这可能与 yolo11n 模型中复杂的CSP结构有关，模型中 ``Concat`` 或 ``Add`` 层的输入可能具有不同的分布或尺度。我们可以选择使用int16对某些层进行量化，并采用算子分裂过程优化量化效果。有关详细信息，请参阅混合精度+算子分裂过程量化测试。

混合精度+算子分裂过程量化测试
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

**量化设置:**

.. code-block:: python

   from ppq.api import get_target_platform
   target="esp32p4"
   num_of_bits=8
   batch_size=32

   # Quantize the following layers with 16-bits
   quant_setting = QuantizationSettingFactory.espdl_setting()
   quant_setting.dispatching_table.append("/model.2/cv2/conv/Conv", get_target_platform(TARGET, 16))
   quant_setting.dispatching_table.append("/model.3/conv/Conv", get_target_platform(TARGET, 16))
   quant_setting.dispatching_table.append("/model.4/cv2/conv/Conv", get_target_platform(TARGET, 16))

   # Horizontal Layer Split Pass
   quant_setting.weight_split = True
   quant_setting.weight_split_setting.method = 'balance'
   quant_setting.weight_split_setting.value_threshold = 1.5
   quant_setting.weight_split_setting.interested_layers = ['/model.0/conv/Conv', '/model.1/conv/Conv']
    

**量化结果:**

.. code-block::

   Layer                                        | NOISE:SIGNAL POWER RATIO 
   /model.10/m/m.0/ffn/ffn.1/conv/Conv:         | ████████████████████ | 24.377%
   /model.10/m/m.0/attn/proj/conv/Conv:         | ███████████████      | 18.398%
   /model.23/cv2.2/cv2.2.1/conv/Conv:           | ███████████████      | 17.757%
   /model.23/cv3.2/cv3.2.0/cv3.2.0.0/conv/Conv: | ██████████████       | 17.049%
   /model.23/cv2.2/cv2.2.0/conv/Conv:           | ██████████████       | 16.775%
   /model.22/m.0/cv3/conv/Conv:                 | █████████████        | 15.333%
   /model.23/cv3.2/cv3.2.0/cv3.2.0.1/conv/Conv: | ████████████         | 14.934%
   /model.23/cv3.0/cv3.0.1/cv3.0.1.1/conv/Conv: | ████████████         | 14.775%
   /model.22/m.0/m/m.1/cv2/conv/Conv:           | ████████████         | 14.482%
   /model.23/cv3.2/cv3.2.1/cv3.2.1.1/conv/Conv: | ███████████          | 13.772%
   /model.22/cv2/conv/Conv:                     | ███████████          | 13.712%
   /model.22/m.0/cv2/conv/Conv:                 | ███████████          | 13.618%
   /model.23/cv3.1/cv3.1.1/cv3.1.1.1/conv/Conv: | ███████████          | 13.242%
   /model.23/cv2.1/cv2.1.1/conv/Conv:           | █████████            | 10.791%
   /model.23/cv2.0/cv2.0.1/conv/Conv:           | ████████             | 9.906%
   /model.23/cv2.1/cv2.1.0/conv/Conv:           | ████████             | 9.613%
   /model.22/cv1/conv/Conv:                     | ███████              | 8.870%
   /model.10/m/m.0/attn/MatMul_1:               | ███████              | 8.179%
   /model.23/cv2.2/cv2.2.2/Conv:                | ███████              | 8.137%
   /model.22/m.0/m/m.0/cv2/conv/Conv:           | ███████              | 8.071%
   /model.10/m/m.0/attn/qkv/conv/Conv:          | ██████               | 7.823%
   /model.23/cv3.1/cv3.1.0/cv3.1.0.1/conv/Conv: | ██████               | 7.799%
   /model.13/m.0/cv2/conv/Conv:                 | ██████               | 7.522%
   /model.19/m.0/cv2/conv/Conv:                 | ██████               | 7.233%
   /model.20/conv/Conv:                         | ██████               | 7.027%
   /model.23/cv3.2/cv3.2.1/cv3.2.1.0/conv/Conv: | ██████               | 6.960%
   /model.10/m/m.0/attn/pe/conv/Conv:           | ██████               | 6.825%
   /model.23/cv3.0/cv3.0.0/cv3.0.0.1/conv/Conv: | █████                | 6.693%
   /model.22/m.0/m/m.1/cv1/conv/Conv:           | █████                | 6.444%
   /model.22/m.0/m/m.0/cv1/conv/Conv:           | █████                | 6.266%
   /model.19/cv2/conv/Conv:                     | █████                | 6.129%
   /model.13/cv2/conv/Conv:                     | █████                | 5.778%
   /model.10/cv1/conv/Conv:                     | █████                | 5.756%
   /model.10/cv2/conv/Conv:                     | █████                | 5.602%
   /model.19/cv1/conv/Conv:                     | ████                 | 5.181%
   /model.19/m.0/cv1/conv/Conv:                 | ████                 | 4.959%
   /model.22/m.0/cv1/conv/Conv:                 | ████                 | 4.925%
   /model.23/cv3.1/cv3.1.1/cv3.1.1.0/conv/Conv: | ████                 | 4.911%
   /model.8/m.0/cv2/conv/Conv:                  | ████                 | 4.871%
   /model.10/m/m.0/attn/MatMul:                 | ████                 | 4.621%
   /model.13/cv1/conv/Conv:                     | ████                 | 4.507%
   /model.23/cv2.0/cv2.0.0/conv/Conv:           | ████                 | 4.369%
   /model.23/cv2.0/cv2.0.2/Conv:                | ███                  | 4.262%
   /model.13/m.0/cv1/conv/Conv:                 | ███                  | 4.187%
   /model.23/cv2.1/cv2.1.2/Conv:                | ███                  | 4.150%
   /model.6/m.0/cv2/conv/Conv:                  | ███                  | 4.035%
   /model.17/conv/Conv:                         | ███                  | 3.741%
   /model.16/m.0/cv2/conv/Conv:                 | ███                  | 3.456%
   /model.8/m.0/cv1/conv/Conv:                  | ███                  | 3.226%
   /model.23/cv3.1/cv3.1.0/cv3.1.0.0/conv/Conv: | ███                  | 3.145%
   /model.23/cv3.0/cv3.0.1/cv3.0.1.0/conv/Conv: | ███                  | 3.140%
   /model.16/m.0/cv1/conv/Conv:                 | ███                  | 3.137%
   /model.8/m.0/cv3/conv/Conv:                  | ██                   | 3.077%
   /model.8/cv2/conv/Conv:                      | ██                   | 3.074%
   /model.8/m.0/m/m.1/cv1/conv/Conv:            | ██                   | 3.058%
   /model.2/cv2/conv/Conv:                      | ██                   | 3.053%
   /model.6/m.0/cv3/conv/Conv:                  | ██                   | 3.048%
   /model.16/cv2/conv/Conv:                     | ██                   | 3.015%
   /model.8/cv1/conv/Conv:                      | ██                   | 2.982%
   /model.8/m.0/m/m.0/cv2/conv/Conv:            | ██                   | 2.948%
   /model.6/cv1/conv/Conv:                      | ██                   | 2.782%
   /model.8/m.0/m/m.0/cv1/conv/Conv:            | ██                   | 2.743%
   /model.10/m/m.0/ffn/ffn.0/conv/Conv:         | ██                   | 2.708%
   /model.2/cv1/conv/Conv:                      | ██                   | 2.697%
   /model.6/cv2/conv/Conv:                      | ██                   | 2.616%
   /model.8/m.0/m/m.1/cv2/conv/Conv:            | ██                   | 2.611%
   /model.9/cv2/conv/Conv:                      | ██                   | 2.505%
   /model.3/conv/Conv:                          | ██                   | 2.500%
   /model.2/m.0/cv2/conv/Conv:                  | ██                   | 2.470%
   /model.6/m.0/m/m.0/cv1/conv/Conv:            | ██                   | 2.236%
   /model.6/m.0/m/m.0/cv2/conv/Conv:            | ██                   | 2.231%
   /model.4/cv2/conv/Conv:                      | ██                   | 2.152%
   /model.7/conv/Conv:                          | ██                   | 2.076%
   /model.6/m.0/m/m.1/cv1/conv/Conv:            | ██                   | 2.070%
   /model.5/conv/Conv:                          | ██                   | 1.999%
   /model.16/cv1/conv/Conv:                     | █                    | 1.879%
   /model.4/cv1/conv/Conv:                      | █                    | 1.807%
   /model.4/m.0/cv1/conv/Conv:                  | █                    | 1.741%
   /model.6/m.0/cv1/conv/Conv:                  | █                    | 1.734%
   /model.6/m.0/m/m.1/cv2/conv/Conv:            | █                    | 1.527%
   /model.4/m.0/cv2/conv/Conv:                  | █                    | 1.249%
   /model.23/cv3.0/cv3.0.0/cv3.0.0.0/conv/Conv: | █                    | 0.873%
   /model.1/conv/Conv:                          | █                    | 0.781%
   /model.23/cv3.2/cv3.2.2/Conv:                | █                    | 0.766%
   PPQ_Operation_2:                             |                      | 0.698%
   /model.9/cv1/conv/Conv:                      |                      | 0.681%
   /model.2/m.0/cv1/conv/Conv:                  |                      | 0.508%
   /model.23/cv3.1/cv3.1.2/Conv:                |                      | 0.349%
   /model.23/cv3.0/cv3.0.2/Conv:                |                      | 0.188%
   PPQ_Operation_0:                             |                      | 0.110%
   /model.0/conv/Conv:                          |                      | 0.099%

   Analysing Layerwise quantization error:: 100%|██████████| 91/91 [05:06<00:00,  3.37s/it]
   Layer                                        | NOISE:SIGNAL POWER RATIO 
   /model.22/cv1/conv/Conv:                     | ████████████████████ | 0.244%
   /model.9/cv2/conv/Conv:                      | █████████████        | 0.156%
   /model.10/cv1/conv/Conv:                     | ███████████          | 0.132%
   /model.1/conv/Conv:                          | ██████               | 0.077%
   /model.4/cv1/conv/Conv:                      | ██████               | 0.074%
   /model.16/cv1/conv/Conv:                     | █████                | 0.066%
   /model.0/conv/Conv:                          | █████                | 0.061%
   /model.2/cv1/conv/Conv:                      | █████                | 0.060%
   /model.23/cv2.0/cv2.0.0/conv/Conv:           | ████                 | 0.052%
   PPQ_Operation_0:                             | ████                 | 0.047%
   /model.2/m.0/cv1/conv/Conv:                  | ████                 | 0.045%
   /model.10/m/m.0/attn/pe/conv/Conv:           | ██                   | 0.029%
   /model.2/m.0/cv2/conv/Conv:                  | ██                   | 0.029%
   /model.10/m/m.0/attn/MatMul:                 | ██                   | 0.025%
   /model.6/cv1/conv/Conv:                      | ██                   | 0.025%
   /model.22/m.0/m/m.0/cv1/conv/Conv:           | ██                   | 0.023%
   /model.16/cv2/conv/Conv:                     | ██                   | 0.021%
   /model.16/m.0/cv2/conv/Conv:                 | ██                   | 0.020%
   /model.19/m.0/cv1/conv/Conv:                 | ██                   | 0.020%
   /model.4/m.0/cv1/conv/Conv:                  | █                    | 0.018%
   /model.19/cv2/conv/Conv:                     | █                    | 0.017%
   /model.4/m.0/cv2/conv/Conv:                  | █                    | 0.016%
   /model.10/m/m.0/attn/qkv/conv/Conv:          | █                    | 0.016%
   /model.19/cv1/conv/Conv:                     | █                    | 0.015%
   /model.13/cv2/conv/Conv:                     | █                    | 0.015%
   /model.23/cv2.1/cv2.1.0/conv/Conv:           | █                    | 0.013%
   /model.23/cv2.2/cv2.2.1/conv/Conv:           | █                    | 0.012%
   /model.13/cv1/conv/Conv:                     | █                    | 0.012%
   /model.6/cv2/conv/Conv:                      | █                    | 0.011%
   /model.13/m.0/cv1/conv/Conv:                 | █                    | 0.011%
   /model.8/cv1/conv/Conv:                      | █                    | 0.010%
   /model.13/m.0/cv2/conv/Conv:                 | █                    | 0.010%
   /model.5/conv/Conv:                          | █                    | 0.010%
   /model.6/m.0/m/m.1/cv1/conv/Conv:            | █                    | 0.009%
   /model.23/cv3.0/cv3.0.0/cv3.0.0.1/conv/Conv: | █                    | 0.008%
   /model.23/cv2.2/cv2.2.0/conv/Conv:           | █                    | 0.008%
   /model.23/cv2.1/cv2.1.1/conv/Conv:           | █                    | 0.008%
   /model.19/m.0/cv2/conv/Conv:                 | █                    | 0.008%
   /model.8/cv2/conv/Conv:                      | █                    | 0.008%
   /model.9/cv1/conv/Conv:                      | █                    | 0.008%
   /model.23/cv2.0/cv2.0.1/conv/Conv:           | █                    | 0.007%
   /model.16/m.0/cv1/conv/Conv:                 | █                    | 0.007%
   /model.17/conv/Conv:                         | █                    | 0.007%
   /model.23/cv3.1/cv3.1.1/cv3.1.1.0/conv/Conv: | █                    | 0.007%
   /model.10/m/m.0/ffn/ffn.1/conv/Conv:         | █                    | 0.007%
   /model.22/m.0/cv1/conv/Conv:                 |                      | 0.006%
   /model.10/cv2/conv/Conv:                     |                      | 0.006%
   /model.23/cv2.0/cv2.0.2/Conv:                |                      | 0.006%
   /model.23/cv2.2/cv2.2.2/Conv:                |                      | 0.005%
   /model.23/cv2.1/cv2.1.2/Conv:                |                      | 0.005%
   /model.22/m.0/cv3/conv/Conv:                 |                      | 0.005%
   /model.23/cv3.1/cv3.1.0/cv3.1.0.1/conv/Conv: |                      | 0.005%
   /model.22/cv2/conv/Conv:                     |                      | 0.005%
   /model.7/conv/Conv:                          |                      | 0.004%
   /model.6/m.0/cv3/conv/Conv:                  |                      | 0.004%
   /model.10/m/m.0/ffn/ffn.0/conv/Conv:         |                      | 0.004%
   /model.8/m.0/m/m.1/cv2/conv/Conv:            |                      | 0.004%
   /model.22/m.0/m/m.1/cv1/conv/Conv:           |                      | 0.004%
   /model.8/m.0/m/m.1/cv1/conv/Conv:            |                      | 0.004%
   /model.23/cv3.1/cv3.1.1/cv3.1.1.1/conv/Conv: |                      | 0.003%
   /model.8/m.0/cv1/conv/Conv:                  |                      | 0.003%
   /model.10/m/m.0/attn/proj/conv/Conv:         |                      | 0.003%
   /model.22/m.0/m/m.0/cv2/conv/Conv:           |                      | 0.003%
   PPQ_Operation_2:                             |                      | 0.003%
   /model.8/m.0/cv3/conv/Conv:                  |                      | 0.003%
   /model.6/m.0/m/m.0/cv1/conv/Conv:            |                      | 0.003%
   /model.23/cv3.2/cv3.2.1/cv3.2.1.0/conv/Conv: |                      | 0.002%
   /model.6/m.0/m/m.1/cv2/conv/Conv:            |                      | 0.002%
   /model.8/m.0/m/m.0/cv2/conv/Conv:            |                      | 0.002%
   /model.23/cv3.0/cv3.0.0/cv3.0.0.0/conv/Conv: |                      | 0.002%
   /model.23/cv3.2/cv3.2.1/cv3.2.1.1/conv/Conv: |                      | 0.002%
   /model.10/m/m.0/attn/MatMul_1:               |                      | 0.002%
   /model.22/m.0/m/m.1/cv2/conv/Conv:           |                      | 0.001%
   /model.6/m.0/m/m.0/cv2/conv/Conv:            |                      | 0.001%
   /model.8/m.0/m/m.0/cv1/conv/Conv:            |                      | 0.001%
   /model.23/cv3.0/cv3.0.1/cv3.0.1.0/conv/Conv: |                      | 0.001%
   /model.23/cv3.2/cv3.2.0/cv3.2.0.1/conv/Conv: |                      | 0.001%
   /model.2/cv2/conv/Conv:                      |                      | 0.001%
   /model.23/cv3.0/cv3.0.1/cv3.0.1.1/conv/Conv: |                      | 0.001%
   /model.6/m.0/cv1/conv/Conv:                  |                      | 0.001%
   /model.23/cv3.2/cv3.2.2/Conv:                |                      | 0.001%
   /model.20/conv/Conv:                         |                      | 0.001%
   /model.23/cv3.1/cv3.1.2/Conv:                |                      | 0.001%
   /model.23/cv3.2/cv3.2.0/cv3.2.0.0/conv/Conv: |                      | 0.001%
   /model.6/m.0/cv2/conv/Conv:                  |                      | 0.001%
   /model.23/cv3.0/cv3.0.2/Conv:                |                      | 0.000%
   /model.23/cv3.1/cv3.1.0/cv3.1.0.0/conv/Conv: |                      | 0.000%
   /model.8/m.0/cv2/conv/Conv:                  |                      | 0.000%
   /model.22/m.0/cv2/conv/Conv:                 |                      | 0.000%
   /model.3/conv/Conv:                          |                      | 0.000%
   /model.4/cv2/conv/Conv:                      |                      | 0.000%

**量化误差分析:**

在对逐层误差较高的层使用16-bit量化，并采用算子分裂过程后，在相同输入下，量化后的模型在 COCO val2017 上的 mAP50:95 提升至33.4%；同时可以观察到输出层的累计误差明显减少。

模型的输出层/model.23/cv3.2/cv3.2.2/Conv， /model.23/cv2.2/cv2.2.2/Conv， /model.23/cv3.1/cv3.1.2/Conv， /model.23/cv2.1/cv2.1.2/Conv， /model.23/cv3.0/cv3.0.2/Conv和/model.23/cv2.0/cv2.0.2/Conv的累计误差分别为0.766%，8.137%，0.349%，4.150%，0.188%和4.262%。
   
模型部署
-----------

:project:`参考示例 <examples/yolo11_detect>`

目标检测基类
^^^^^^^^^^^^^^^^^

- :project_file:`dl_detect_base.hpp <esp-dl/vision/detect/dl_detect_base.hpp>`
- :project_file:`dl_detect_base.cpp <esp-dl/vision/detect/dl_detect_base.cpp>`

前处理
^^^^^^^^^

``ImagePreprocessor`` 类中封装了常用的图像前处理流程，包括 ``color conversion``, ``crop``, ``resize``, ``normalization``, ``quantize``。

- :project_file:`dl_image_preprocessor.hpp <esp-dl/vision/image/dl_image_preprocessor.hpp>`
- :project_file:`dl_image_preprocessor.cpp <esp-dl/vision/image/dl_image_preprocessor.cpp>`

后处理
^^^^^^^^^

- :project_file:`dl_detect_postprocessor.hpp <esp-dl/vision/detect/dl_detect_postprocessor.hpp>`
- :project_file:`dl_detect_postprocessor.cpp <esp-dl/vision/detect/dl_detect_postprocessor.cpp>`
- :project_file:`dl_detect_yolo11_postprocessor.hpp <esp-dl/vision/detect/dl_detect_yolo11_postprocessor.hpp>`
- :project_file:`dl_detect_yolo11_postprocessor.cpp <esp-dl/vision/detect/dl_detect_yolo11_postprocessor.cpp>`

