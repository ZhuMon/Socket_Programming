# Socket_Programming
## main架構
* 藉由stat函數存取檔案大小
  ![](https://i.imgur.com/aKyaxXo.png)
    ![](https://i.imgur.com/EwTWkPe.png)

* 將TCP、UDP的recv、send分別寫成四個函數：
	```tcp_s(), tcp_c(), udp_s(), udp_c()```
* 藉由判斷argv來決定執行哪一個函數
	* tcp recv → tcp_s()
	* tcp send → tcp_c()
	* udp recv → udp_s()
	* udp send → udp_c()
![](https://i.imgur.com/8CFnnK1.png) 
## TCP 架構
### tcp_c()
1. 先傳送一個包含以下資訊的封包
   * 檔案名稱
   * 檔案大小
    > 為了讓檔案完整需要檔案大小
    * 將檔案名稱(```file_name```)放到封包的第一行
    * 將檔案大小(```file_size_c```)放到封包的第二行
    * 以```char f_buffer[256]```紀錄第一個封包
    * 以```strcat()```合併字串到```f_buffer```
	
    ![](https://i.imgur.com/hjccquz.png)
2. 計算封包數量
    * 將 file 分成 20 個 segment，每個 segment 到達 receive 端時，便代表已傳送 5%
    * 因為若只將 file 分成 20 個 packets，file 太大時，每個 packets 的 bytes 數會太大，導致接收後檔案出錯
    * 所以若是將 file size 除以 20 後，發現超過 256 bytes，便分成 20 個 packet team，每個 packet team 包含數個 packet
    * ![](https://i.imgur.com/xpK4fne.png)

3. 確定要分成 20 個 segment 還是 21 個
    * 若是檔案大小為 681 bytes
        * 若只分成 20 等分：681 = 19(個) * 35(bytes) + 1(個) * 16(bytes)
        * ![](https://i.imgur.com/lnZtIHd.png)
        * 可以發現到中間就會開始失真，傳送到的資料非5%
        
        ---
        * 但若分成 21 等分：681 = 20(個) * 34(bytes) + 1(個) * 1(bytes)
        * ![](https://i.imgur.com/hHhRL6n.png)
        * 會發現幾乎符合每個封包大小為 5% 的 file 大小

    * ![](https://i.imgur.com/jDghp2q.png)

4. 讀檔
    * 宣告一個跟檔案大小一樣大的 char 存取整個檔案
    * ![](https://i.imgur.com/aVS9i2v.png)

5. 傳送封包
    * 將buffer(儲存file)解析到p_buffer(儲存packet)
    * 若是只有20或21個 packets
    ![](https://i.imgur.com/G0ixdAM.png)
    * 若是有team
    * ![](https://i.imgur.com/vK53mpS.png)
![](https://i.imgur.com/zTsXCQm.png)
![](https://i.imgur.com/tmrdgmL.png)

6. 接收"完成"訊息
---
### tcp_s()
1. 解析第一個封包
    * 以換行符```\n```來解析封包，並且以```line_pointer```紀錄```\n```在buffer的位置
    ![](https://i.imgur.com/KZRvBnZ.png)
2. 計算封包數量
    * 同tcp_c()
    * 已接收到的訊息(```file_size```)來計算
3. 確定要分成 20 個 segment 還是 21 個
    * 同tcp_c()
    * 已接收到的訊息(```file_size```)來計算
4. 接收封包
    * 將tcp_c()處的傳送改成接收，並且不解析buffer(```p_buffer[i] = buffer[i]```)
    * 寫入檔案，並且在每接收5%封包時，紀錄時間並印在螢幕上
5. 傳送"完成"訊息

---

## UDP 架構
### udp_c()
* 類似tcp_c()，只是傳輸packet後，要收到相同的packet，才繼續往下做
### udp_s()
* 類似udp_s()，只是收到packet後，要傳回相同的packet
---
## 實驗結果
## TCP
### 傳輸txt
* mac OSX -> Ubuntu : 傳送big.txt
  > 傳之前要把虛擬機第一個網路介面卡改成「僅限主機」介面卡
    * Ubuntu:
        * ![](https://i.imgur.com/UmgKTAP.png)
        * ![](https://i.imgur.com/YN9ULXh.png)
    * mac OSX:
        * ![](https://i.imgur.com/BIPW6am.png)


* Ubuntu -> mac OSX : 傳送big.txt
  > 傳之前要把虛擬機第一個網路介面卡改成NAT
    * Ubuntu:
        * ![](https://i.imgur.com/e123kMq.png)

    * mac OSX:
        * ![](https://i.imgur.com/rqAHBUG.png)
        * ![](https://i.imgur.com/FxOX3R3.png)
        
    

---

### 傳輸圖片
* mac OSX -> Ubuntu : 傳送IR_transister.JPG
    > 傳之前要把虛擬機第一個網路介面卡改成「僅限主機」介面卡
    * Ubuntu:
        *    ![](https://i.imgur.com/W1jqVVo.png)
        *    ![](https://i.imgur.com/vXmFZZu.png)
    * mac OSX:
      ![](https://i.imgur.com/ItmqCH8.png)

* Ubuntu -> mac OSX : 傳送fire.jpeg
    > 傳之前要把虛擬機第一個網路介面卡改成NAT
    * Ubuntu:
        * ![](https://i.imgur.com/3Gsa1Ub.png)

    * mac OSX:
        * ![](https://i.imgur.com/SWjkouX.png)
        * ![](https://i.imgur.com/rg44rui.png)
        > 不小心打成ps
    ---

## UDP
### 傳輸txt
* mac OSX -> Ubuntu : 傳送big.txt
 
    * Ubuntu:
        * ![](https://i.imgur.com/bx9x0Dy.png)
            ![](https://i.imgur.com/FV8sxhf.png)
        * ![](https://i.imgur.com/ZUj9wq3.png)
    * mac OSX:
        * ![](https://i.imgur.com/2WyKglC.png)

### 傳輸圖片
* mac OSX -> Ubuntu : 傳送IR_transister.JPG
  
    * Ubuntu:
        * ![](https://i.imgur.com/xVJbX0e.png)
        ![](https://i.imgur.com/I1pwzir.png) 
        * 圖片：![](https://i.imgur.com/Bb6r99R.png)



    * mac OSX:
        * ![](https://i.imgur.com/j8THjuB.png)


