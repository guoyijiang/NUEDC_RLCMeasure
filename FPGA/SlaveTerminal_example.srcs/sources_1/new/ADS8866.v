//T = 10610ns F = 94.2501KSPS
module AlwaysReadADS8865(
	input wire clk,
	input wire rst,
	output wire[15:0] ADReceiveData,
	output wire SampleFinished,
	output wire CS_AD,
	output wire SCLK_AD,
	input wire MISO_AD
	
);
	reg adSampleEn;
	
	//不停采样
	
	//AD采样的延时
	reg [9:0]adDelayCnt;
	always@(posedge clk) begin
		if(~rst)
		begin
			adDelayCnt <= 10'd0;
			adSampleEn <= 1'b1;
		end
		else 
		begin
			if(SampleFinished)
				adDelayCnt <= 10'd880;
			else if(adDelayCnt != 10'd0) adDelayCnt <= adDelayCnt -10'd1;
			if(adDelayCnt == 10'd1)	adSampleEn <= 1'b1;
			else adSampleEn <= 1'b0;
		end
	end
	
	
	//实例化AD //
	my_spi_module  my_spi_module_ad(
 	.clk(clk),
	.rst(rst),
	.data_out_valid(adSampleEn),
	
	.DATA_WITH(5'd15),// = n-1
	.DATA_EDGE_NUM(6'd31),// = n*2-1
	.SPI_CLK_INIT(1'b0),
	.DATA_OUT_EDGE(1'b0),	//下降沿写数据（上升沿读数据）
	
	.CS_DOWN_nclk(16'd48),// = n-2
	.CS_UP_nclk(16'd40),// = n-2
	.SPI_HALF_PERIOD_nclk(16'd40),// = n-1
	
	.SPI_DATA_OUT(32'h0000dddd),//方便测试1101110111011101
	.SPI_DATA_IN(ADReceiveData),
	.SPI_STATE(),

	.SPI_CLK(SCLK_AD),
	.MOSI(),
	.MISO(MISO_AD),
	.CS(CS_AD)
	);
	
	//捕获CS上升沿
	reg[1:0] csad;
	always@(posedge clk) begin
		if(~rst)
			csad <= 2'b11;
		else 
			csad<= {csad[0],CS_AD};
	end
	assign SampleFinished = (csad == 2'b01);

	
endmodule
