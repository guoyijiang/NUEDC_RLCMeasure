

module my_spi_module (
 	input wire clk,
	input wire rst,
	input wire data_out_valid,
	
	input  wire [4:0]DATA_WITH,// = n-1
	input  wire [5:0]DATA_EDGE_NUM,// = n*2-1
	input  wire SPI_CLK_INIT,
	input  wire DATA_OUT_EDGE,	
	
	input  wire [15:0]CS_DOWN_nclk,// = n-2
	input  wire [15:0]CS_UP_nclk,// = n-2
	input  wire [15:0]SPI_HALF_PERIOD_nclk,// = n-1
	
	input wire  [31: 0] SPI_DATA_OUT,
	output wire [31:0]SPI_DATA_IN,
	output wire [31: 0] SPI_STATE,

	output wire SPI_CLK,
	output wire MOSI,
	input  wire MISO,
	output wire CS
	);
	
	//data_out_valid > cs,cs_down_cnting > cs_down_valid
	//cs_down_valid > cs_cnt_complete_en > spi_idle,SPI_IDLE_NEGEDGE
	//SPI_IDLE_NEGEDGE > spi_clk_cnting
	//spi_clk_cnting > clk
	//clk > SPI_CLK_POSEDGE,SPI_CLK_NEGEDGE
	//SPI_CLK_POSEDGE,SPI_CLK_NEGEDGE > MISO, MOSI
	//spi_clk_cnting_over >spi_idle,SPI_IDLE_POSEDGE
	//SPI_IDLE_POSEDGE > data_complete_valid
	//data_complete_valid > cs_up_cnting
	//cs_cnt_complete_en > cs

	wire SPI_CLK_POSEDGE;
	wire SPI_CLK_NEGEDGE;
	wire SPI_IDLE_NEGEDGE;
	wire SPI_IDLE_POSEDGE;

	reg [31: 0] spi_reg;
	reg [31: 0] spi_data_in;

	reg mosi;
	reg cs;
	
	reg spi_clk;
	reg spi_clk_last;
	reg spi_clk_cnting;
	reg spi_clk_cnting_last;
	reg spi_clk_cnting_over;
	
	reg [15:0]cnt_half_clk; // f_spi> 1.6KHz tnclk_spi <65535
	reg [5:0]spi_clk_edge_cnt;//<64
			
	reg [15:0] cs_half_clk_cnt;
	
	reg spi_idle;
	reg spi_idle_last;
	
	reg data_complete_valid;
	
	reg cs_down_cnting;
	reg cs_up_cnting;
	
	reg	cs_down_valid;
	reg cs_cnt_complete_en;
		
	
	//cs cnt
	always@(posedge clk)
	begin
		if(~rst)
		begin
			cs_cnt_complete_en <= 1'b0;
			cs_half_clk_cnt <= 0;		
		end
		else
		begin
			if(cs_cnt_complete_en) cs_cnt_complete_en <= 1'b0;
			if(cs_down_cnting | cs_up_cnting)
			begin
				if(cs_half_clk_cnt < CS_DOWN_nclk) cs_half_clk_cnt <= cs_half_clk_cnt + 1'b1;
				else
				begin
				cs_half_clk_cnt <= 0;
				cs_cnt_complete_en <= 1'b1;
				end
			end	
			else cs_half_clk_cnt <= 0;
		end
	end

	assign CS = cs;
	//cs
	always@ (posedge clk)
	begin
		if(~rst)
		begin
			cs <= 1'b1;
			cs_down_cnting <= 1'b0;
			cs_up_cnting <= 1'b0;	
			cs_down_valid <= 1'b0;
		end
		else begin
			if(data_out_valid)
			begin
				cs <= 1'b0;
				cs_down_cnting <= 1'b1;			
			end
			else if(data_complete_valid)
			begin
				cs_up_cnting <= 1'b1;
			end
			
			if(cs_down_cnting & cs_cnt_complete_en)
			begin
				cs_down_valid <= 1'b1;
				cs_down_cnting <= 1'b0;		
			end
			else begin
				cs_down_valid <= 1'b0;
				if(cs_up_cnting & cs_cnt_complete_en)
				begin
					cs <= 1'b1;
					cs_up_cnting <= 1'b0;
				end		
			end
		end
	end
	
	//generate spi_clk
	assign SPI_CLK = spi_clk;
	always@(posedge clk)
	begin
		if(~rst)
		begin
			cnt_half_clk <= 0;
			spi_clk <= SPI_CLK_INIT;
			spi_clk_edge_cnt <= 0;
			spi_clk_cnting <= 1'b0;
			spi_clk_cnting_last <= 1'b0;
		end
		else
		begin
			if(SPI_IDLE_NEGEDGE||spi_clk_cnting)
			begin
				spi_clk_cnting <= 1'b1;
				if(cnt_half_clk < (SPI_HALF_PERIOD_nclk)) cnt_half_clk <= cnt_half_clk + 1'b1;
				else 
				begin
					cnt_half_clk <= 0;
					spi_clk <= ~spi_clk;
					if(spi_clk_edge_cnt < DATA_EDGE_NUM) spi_clk_edge_cnt <= spi_clk_edge_cnt + 1'b1;
					else
					begin
						spi_clk_edge_cnt <= 0;
						spi_clk_cnting <= 1'b0;	
					end	
				end		
			end
			spi_clk_cnting_last <= spi_clk_cnting;
		end
	end
	
	//generate spi_clk_cnting_over
	
	always@(posedge clk)
	begin
		if(~rst) spi_clk_cnting_over = 1'b0;
		else spi_clk_cnting_over <= (~spi_clk_cnting)&&spi_clk_cnting_last;
	end
	
	//generate SPI_CLK_NEGEDGE and SPI_CLK_POSEDGE
	assign SPI_CLK_NEGEDGE = (spi_clk_last == 1'b1)&&(spi_clk == 1'b0);
	assign SPI_CLK_POSEDGE = (spi_clk_last == 1'b0)&&(spi_clk == 1'b1);
	always@(posedge clk)
	begin
		if(~rst) spi_clk_last <= SPI_CLK_INIT;
		else 	spi_clk_last <= spi_clk;
	end
	
	//generate output and input
	assign MOSI = mosi;
	always@(posedge clk) 
	begin
		if(~rst)
		begin
			spi_reg <= 0;
			mosi <= 0;
			spi_idle <= 1'b1;
			spi_idle_last <= 1'b1;
		end
		else
		begin
			if(cs_down_valid && spi_idle ) // load in  SPI_DATA_OUT
			begin
				spi_reg <= SPI_DATA_OUT;
				spi_idle <= 1'b0;
			end
			else if((SPI_CLK_POSEDGE & DATA_OUT_EDGE)||(SPI_CLK_NEGEDGE & (~DATA_OUT_EDGE))&&(~spi_idle))
			begin
				mosi <= spi_reg[DATA_WITH];				
			end
			else if((SPI_CLK_NEGEDGE & DATA_OUT_EDGE)||(SPI_CLK_POSEDGE & (~DATA_OUT_EDGE))&&(~spi_idle))
			begin
				spi_reg <= (spi_reg<<1);	
				spi_reg[0] <= MISO;			    
			end
			  
			if(spi_clk_cnting_over)
			begin
				spi_idle <= 1'b1;			 
			end
			spi_idle_last <= spi_idle;
		end
	end
	
	//Update spi_data_in
	assign SPI_DATA_IN = spi_data_in;
	always@(posedge clk)
	begin
		if(~rst) begin
		spi_data_in <= 0;
		data_complete_valid <=1'b0;
		end
		else if(SPI_IDLE_POSEDGE) begin
		spi_data_in <= spi_reg; // load out spi_data_in
		data_complete_valid <= 1'b1;
		end
		else data_complete_valid <= 1'b0;
	end

	
	
	//generate SPI_IDLE_NEGEDGE and SPI_IDLE_POSEDGE
	
	assign SPI_IDLE_NEGEDGE = (~spi_idle)&&spi_idle_last;
	assign SPI_IDLE_POSEDGE = spi_idle&&(~spi_idle_last);
	
	//generate spi_state;
	//[0:0]  =1 CS
	assign SPI_STATE[0] = CS;
	
endmodule