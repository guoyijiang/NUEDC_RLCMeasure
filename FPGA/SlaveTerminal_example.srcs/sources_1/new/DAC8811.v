//T = 800ns F = 1.25MSPS
module AlwaysWriteDAC8811(
	input clk,
	input rst,
	input wire [15:0] DAPreData,//这里的x是十位的有符号数 经过双极性DA电路可以调到幅值约为2.4V的正弦波
	output wire CS_DA,
	output wire SCLK_DA,	
	output wire MOSI_DA
);
	
    //为DA准备数据
    //DA以1.4MPs写数据 //？？？？？？？线性拟合
	
	reg sendDataEn;//DA写数据使能
	wire SendEnd;//DA CS上升沿
	reg [9:0]daDelayCnt;
	reg[15:0] spiSendData;
	
	
	reg [1:0] dastate;// 0idle;1send;2delay
    always@(posedge clk) begin
        if(~rst)
        begin
            dastate <= 2'd0;
            sendDataEn <= 1'b0; //默认复位开始
			spiSendData <= 16'd0;
        end
        else 
        begin
			case(dastate)
				2'd0:
					begin
						spiSendData <= DAPreData;
						sendDataEn <= 1'b1;	
						dastate <= 2'd1;
					end
				2'd1:
					begin
						sendDataEn <= 1'b0;
						if(SendEnd) dastate <= 2'd2;
					end
				2'd2:
					begin
						if(daDelayCnt < 10'd10)
							daDelayCnt <= daDelayCnt + 10'd1;
						else begin
							daDelayCnt <= 10'd0;		
							spiSendData <= DAPreData;
							sendDataEn <= 1'b1;
							dastate <= 2'd1;
						end
					end
				default:
					begin
						sendDataEn <= 1'b1;
						dastate <= 2'd1;
					end
			endcase 
        end
    end  

    reg[1:0] csda;
    always@(posedge clk) begin
        if(~rst)
            csda <= 2'b11;
        else 
            csda<= {csda[0],CS_DA};
    end
    assign SendEnd = (csda == 2'b01);	
	
	my_spi_module  da_inst(
	.clk(clk),
	.rst(rst),
	.data_out_valid(sendDataEn),
	
	.DATA_WITH(5'd15),// = n-1
	.DATA_EDGE_NUM(6'd31),// = n*2-1
	.SPI_CLK_INIT(1'b1),
	.DATA_OUT_EDGE(1'b0),    
	
	.CS_DOWN_nclk(16'd1),// = n-2
	.CS_UP_nclk(16'd1),// = n-2
	.SPI_HALF_PERIOD_nclk(16'd1),// = n-1
	
	.SPI_DATA_OUT({16'b0,spiSendData}),
	.SPI_DATA_IN(),
	.SPI_STATE(),

	.SPI_CLK(SCLK_DA),
	.MOSI(MOSI_DA),
	.MISO(1'b1),
	.CS(CS_DA)
	);
	
endmodule
