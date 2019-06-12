中文|[英文](README.md)
## 开源项目-Ascend Developer Kit

Ascend Developer Kit（缩写：Ascend DK）是基于华为公司自研的人工智能芯片Ascend 310的计算平台生态项目，提供了丰富的开源代码库，助力开发者进行AI应用程序的开发。Ascend DK可以轻松部署到Atlas 200 Developer Kit（缩写：Atlas 200 DK）中，Atlas 200 DK面向开发者提供一站式开发套件，包括AI硬件计算平台、相关配套工具等，让开发者可以高效编写在特定硬件设备上运行的人工智能应用程序。

## 源码说明

<table>
<tr> 
	<td width="20%">一级目录</td>
	<td width="20%">二级目录</td>
    <td width="20%">三级目录</td>
	<td >说明</td>
</tr>
<tr>
	<td>common</td>
	<td></td>
    <td></td>
	<td></td>
</tr>
<tr>
 	<td></td>
	<td>ascendcamera</td>
    <td></td>
	<td>使用Atlas DK开发板上的摄像头采集数据的样例</td>
</tr>
<tr>
	<td></td>
	<td>presenter</td>
    <td></td>
	<td>推理结果可视化实现样例</td>
</tr>
<tr>
	<td></td>
	<td></td>
	<td>agent</td>
	<td>与Presenter Server交互的API</td>
</tr>
<tr>
	<td></td>
	<td></td>
	<td>server</td>
	<td>展示Presenter Agent推送过来的推理结果，浏览器访问</td>
</tr>
<tr>
	<td></td>
	<td>utils</td>
    <td></td>
	<td>公共能力</td>
</tr>
<tr>
	<td></td>
	<td></td>
    <td>ascend_ezdvpp</td>
	<td>封装了dvpp接口，提供图像/视频的处理能力，如色域转换，图像/视频转换等</td>
</tr>
<tr>
	<td>engine</td>
	<td></td>
    <td></td>
	<td></td>
</tr>
<tr>
	<td></td>
	<td>computervision</td>
    <td></td>
	<td>视觉领域Engine</td>
</tr>
<tr>
	<td>smartcity</td>
	<td></td>
    <td></td>
	<td>ascenddk smart city样例</td>
</tr>
<tr>
	<td></td>
    <td>facedetectionapp</td>
	<td></td>
	<td>人脸检测应用</td>
</tr>
<tr>
	<td></td>
    <td>facialrecognition</td>
	<td></td>
	<td>人脸识别应用</td>
</tr>
<tr>
	<td></td>
    <td>videoanalysis</td>
	<td></td>
	<td>视频结构化应用</td>
</tr>
<tr>
	<td>travisci</td>
	<td></td>
    <td></td>
	<td>持续集成构建脚本</td>
</tr>
</table>

## 源码使用指导
-   [Atlas DK开源App使用指导](https://ascend.github.io/ascenddk-private/doc/cn/samplecode/OverView.html)

## 贡献流程
-   [Contributing](contributing.md)

## 声明
华为Ascend芯片做为开放的AI计算平台，仅提供相关AI计算能力，使能第三方开发者，相关照片、视频等数据均由第三方开发者自行控制，由第三方开发者决定收集数据的内容与种类，华为不涉及任何用户数据的收集和保存。

## 加入我们
* 欢迎提交issue对关心的问题发起讨论，欢迎提交PR参与特性建设
* 如您有合作意向，希望加入Huawei Ascend生态合作伙伴，请发邮件至ascend@huawei.com，或访问[Ascend官网](https://www.huawei.com/minisite/ascend/cn/index.html)，进一步了解详细信息。
