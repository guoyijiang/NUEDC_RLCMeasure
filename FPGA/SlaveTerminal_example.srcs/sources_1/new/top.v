//new sim
`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Create Date: 2017/04/30 14:41:14
//Author:GYJ
// Design Name: terminal
// Target Devices: 
// Tool Versions: vivado 2016.4
// Description: 
//
//////////////////////////////////////////////////////////////////////////////////
/*transmit
* 1 
* +------------+---------------+--------------------------+
* | 01(R)/10(W)|  ...(0000)... |       		addr		  |
* +------------+---------------+--------------------------+
*  	 15 14    		  		  8 7						  0

* 2(W/R)
* +-------------------------------------------------------+
* |			           data[31:16]						  |		
* +-------------------------------------------------------+

* 3(W/R)
* +-------------------------------------------------------+
* |			           data[15:0]						  |	
* +-------------------------------------------------------+
*/
`define nWREG 10//0~23 can write by master
`define nRREG 20
module top(
        input wire clk,
        input wire crystal_clk,
        //spi slave
        input wire CS,   
        input wire SCK,
        output wire MISO,
        input wire MOSI,
        //clk's
        output wire si5351_scl,
        inout wire si5351_sda,
        //LED
        output reg [7:0] led,
        //fast adda
        input wire [7:0] ad9288_a,
        input wire [7:0] ad9288_b,
        output reg [9:0] dac5652a_a,
        output reg [9:0] dac5652a_b,
		
		//user's

		output wire SCLK_DA,
		output wire CS_DA,
		output wire MOSI_DA,
		
		output wire SCLK_AD,
		output wire CS_AD,
		input wire MISO_AD

    );

/*******************************************************************************************/
//RST
    reg rst;
    initial rst <= 1'b0;
    always@(posedge clk) begin
     if(~rst) rst <= 1'b1;
    end
/*******************************************************************************************/
 //CLK
       Si5351_Init clk_init(
       .clk(crystal_clk),
       .sclx(si5351_scl),
       .sda(si5351_sda)
       );
/*******************************************************************************************/
//BUILD STAGE
        wire[15:0] SEND_BUF,RECEIVE_BUF;
        wire[1:0]SPI_STATE;
        reg[31:0] WREG[0:`nWREG];
        reg[31:0] RREG[`nWREG+1:`nRREG];
        //initial WREG
        initial $readmemh("reg_ini.dat", WREG);
        
        //SPI_SLAVE
        //spi_working     SPI_STATE[0]
        //receive_data     SPI_STATE[1]
        my_spi_slave my_spi_slave_inst(
        .clk(clk),
        .rst(rst),
        .SEND_BUF(SEND_BUF),
        .RECEIVE_BUF(RECEIVE_BUF),
        .spi_state(SPI_STATE),
        .SCK(SCK),
        .CS(CS),
        .MOSI(MOSI),
        .MISO(MISO)
        );    
    
        //STATE: receive_cnt
        //RECEIVE: receive_buf[0:2]
        reg[1:0]receive_cnt = 2'b0;
        reg[15:0] receive_buf[0:2];// 2lsb 1msb 0addr
        always@(posedge clk) begin
            if(~rst) begin
                receive_cnt <= 2'b0;
            end
            else begin
                if(SPI_STATE[1]) begin
                    case(receive_cnt)
                        2'd0:                                     
                            if((RECEIVE_BUF[15:8] == 8'b10000000)||(RECEIVE_BUF[15:8] == 8'b01000000) ) begin
                                if(RECEIVE_BUF[7:0] < `nRREG) begin                            
                                    receive_buf[0]<= RECEIVE_BUF;
                                    receive_cnt <= 2'd1;
                                end            
                            end
                        2'd1: begin
                            receive_buf[1]<= RECEIVE_BUF;
                            receive_cnt <= 2'd2;                
                        end
                        2'd2: begin
                            receive_buf[2]<= RECEIVE_BUF;
                            receive_cnt <= 2'd0;
                        end
                        default: receive_cnt<= 2'b0;
                    endcase
                end
            end
        end    
    
        //STATE CHANGE
        reg[1:0] receive_cnt_l;
        wire STEP_0to1,STEP_1to2,STEP_2to0,READ_EN,WRITE_EN;
        always@(posedge clk) receive_cnt_l <= receive_cnt;
        assign STEP_0to1 = (receive_cnt[0]&(~receive_cnt_l[0]));
        assign STEP_1to2 = (receive_cnt[1]&(~receive_cnt_l[1]));
        assign STEP_2to0 = (receive_cnt_l[1]&(~receive_cnt[1]));
        assign READ_EN      = (receive_buf[0][15:8] == 8'b01000000 );
        assign WRITE_EN  = (receive_buf[0][15:8] == 8'b10000000 );
        
        //READ: SEND DATA
        reg[15:0]send_buf;    
        always@(posedge clk) begin
            if(~rst) begin
            send_buf <= 16'b0;    
            end
            else if(READ_EN) begin
                if(STEP_0to1) begin
					if(receive_buf[0][7:0] <= `nWREG)
						send_buf <= WREG[receive_buf[0][7:0]][31:16];
					else if(receive_buf[0][7:0] <=`nRREG)
						send_buf <= RREG[receive_buf[0][7:0]][31:16];
				end
                if(STEP_1to2) begin
					if(receive_buf[0][7:0] <= `nWREG)
						send_buf <= WREG[receive_buf[0][7:0]][15 :0];
					else if(receive_buf[0][7:0] <=`nRREG)
						send_buf <= RREG[receive_buf[0][7:0]][15 :0];
				end				
            end
        end    
        assign SEND_BUF = send_buf;    
        
        //WRITE: UPDATE WREG
        reg[7:0] addr;
        reg reg_valid;
        always@(posedge clk) begin
            if(~rst) begin
                reg_valid <= 1'b0;
                addr <= 8'b0;
            end
            else begin
                if(STEP_2to0 & WRITE_EN & (receive_buf[0][7:0] <= `nWREG)) begin
                    reg_valid <= 1'b1;
                    addr <= receive_buf[0][7:0];
                    WREG[receive_buf[0][7:0]] <= {receive_buf[1],receive_buf[2]};
                end
                else reg_valid <= 1'b0;
            end
        end
 /*******************************************************************************************/
 //USER'S LOGIC
 //reg_valid 写寄存器有效使能
 //addr 触发写使能的地址
 
 //测试用 占用WREG[0] WREG[1] RREG[11]
 //DDS 占用WREG2 
 //高速DA WREG3
 //DAC8811 占用WREG4 WREG5
 //ADS8860 占用RREG12

 
//测试用 占用WREG[0] WREG[1] RREG[11]
     reg[31:0]sum;
     always@(posedge clk) begin
         if(~rst) begin
             sum <= 32'b0;
         end
         else begin
             if(reg_valid && (addr == 8'd0)) sum <= sum + WREG[0];
             if(reg_valid && (addr == 8'd1)&&(sum >= WREG[1])) sum <= sum - WREG[1];
             RREG[11] <= sum;        
         end
     end
     
//DDS 占用WREG2 WREG3
    wire  signed [9 : 0] ddsOut;
    wire  signed [9 : 0] x;
    wire  signed [9 : 0] y;
    //DDS参数
    reg[31:0] freqw;
    always@(posedge clk) 
    begin
        if(~rst)
            freqw <= 31'd429497;//默认1k
    else
       if(reg_valid && (addr == 8'd2)) freqw <= WREG[2];     
    end    
    //实例化
    dds2DA#(
    .PHASE_W(32),
    .DATA_W(10),
    .TABLE_AW(16),
    .MEM_FILE("SineTable.dat")
    ) dds_inst(
    .FreqWord(freqw), // 
    .doPhaseShift(WREG[3][15:0]),
    .Clock(clk),
    .Out(ddsOut),
    .x(x),
    .y(y)
    );
//DAC8811  占用WREG[4] WREG[5] WREG[6] 
    //WREG[5] 为32位有符号数 0~65536 对应0~1倍幅度；
    //WREG[6]为32位有符号数 -512~511为偏置调整范围，对应-Vref~Vref；
      //DA数据
      
        wire signed [9:0] DAPreData0;
        wire [9:0] DAPreData1;
        wire [15:0] DAPreData;
        assign DAPreData0 = ((WREG[4] == 32'd0)? x:y)*$signed(WREG[5])>>16;// 调幅
        assign DAPreData1 = 32'sd512 + $signed(WREG[6]); //偏置
        assign DAPreData = {DAPreData0 + DAPreData1,6'b0}; //映射
       // assign DAPreData = DAPreData1 + 10'b10_0000_0000;
        
        //DA实例化
        AlwaysWriteDAC8811 AlwaysWriteDAC8811_inst(
            .clk(clk),
            .rst(rst),
            .DAPreData(DAPreData),
            .CS_DA(CS_DA),
            .SCLK_DA(SCLK_DA),    
            .MOSI_DA(MOSI_DA)
        ); 
/*******************************************************************************************/   
wire [15:0] ADReceiveData;
//高速ADDA  占用WREG[7]  WREG[8]  
//WREG[7] 为32位有符号数 0~65536 对应0~1倍幅度；
//WREG[8]为32位有符号数 -512~511为偏置调整范围，对应-Vref~Vref；
wire signed [9:0] fDAPreData0;
wire [9:0] fDAPreData1;
wire [15:0] fDAPreData;
assign fDAPreData0 = ddsOut*$signed(WREG[7])>>16;// 调幅
assign fDAPreData1 = 32'sd512 + $signed(WREG[8]); //偏置
assign fDAPreData = fDAPreData0 + fDAPreData1; //映射
wire [9:0] fDAPreDatab;
assign fDAPreDatab = DAPreData0 + DAPreData1; //b通道跟随精密DA

reg signed [7:0] data_a,data_b;

    always@(posedge clk)begin
        data_a <= ad9288_a;
        data_b <= ad9288_b;
        dac5652a_a[9:0] <= fDAPreData; //消除偏置
        //dac5652a_b[9:0] <= ADReceiveData[15:6];
        dac5652a_b[9:0] <= fDAPreDatab;
    end
	
//ADS8860 占用RREG12

wire SampleFinished;   
AlwaysReadADS8860 AlwaysReadADS8860_inst(
	.clk(clk),
	.rst(rst),
	.ADReceiveData(ADReceiveData),
	.SampleFinished(SampleFinished),
	.CS_AD(CS_AD),
	.SCLK_AD(SCLK_AD),
	.MISO_AD(MISO_AD)
);
//AD数据
always@(posedge clk) begin
     RREG[12] <= ADReceiveData;
end  
 
endmodule
