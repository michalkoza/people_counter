// File: a5_homelien.v
// Generated by MyHDL 0.6
// Date: Sun Jan 18 02:22:21 2009

`timescale 1ns/10ps

module a5_homelien (
    clk,
    i,
    o
);

input clk;
input [2:0] i;
output o;
wire o;

wire r1_out;
reg [18:0] r1_reg;
wire r1_bit;
wire r3_out;
wire r2_clk;
reg [21:0] r2_reg;
wire r3_bit;
wire majority;
wire r3_clk;
reg [22:0] r3_reg;
wire r2_bit;
wire r2_out;
wire r1_clk;



always @(posedge clk) begin: A5_HOMELIEN_R1_1
    if (i[2]) begin
        r1_reg <= 0;
    end
    else if ((i[1] || (majority == r1_reg[8]))) begin
        r1_reg <= {r1_reg[(19 - 1)-1:0], r1_bit};
    end
end


assign r1_clk = r1_reg[8];
assign r1_out = r1_reg[(19 - 1)];

always @(posedge clk) begin: A5_HOMELIEN_R2_1
    if (i[2]) begin
        r2_reg <= 0;
    end
    else if ((i[1] || (majority == r2_reg[10]))) begin
        r2_reg <= {r2_reg[(22 - 1)-1:0], r2_bit};
    end
end


assign r2_clk = r2_reg[10];
assign r2_out = r2_reg[(22 - 1)];

always @(posedge clk) begin: A5_HOMELIEN_R3_1
    if (i[2]) begin
        r3_reg <= 0;
    end
    else if ((i[1] || (majority == r3_reg[10]))) begin
        r3_reg <= {r3_reg[(23 - 1)-1:0], r3_bit};
    end
end


assign r3_clk = r3_reg[10];
assign r3_out = r3_reg[(23 - 1)];


assign majority = (((r1_clk + r2_clk) + r3_clk) >= 2);
assign o = (r1_out ^ r2_out ^ r3_out);


assign r1_bit = (i[0] ^ r1_reg[13] ^ r1_reg[16] ^ r1_reg[17] ^ r1_reg[18]);
assign r2_bit = (i[0] ^ r2_reg[20] ^ r2_reg[21]);
assign r3_bit = (i[0] ^ r3_reg[7] ^ r3_reg[20] ^ r3_reg[21] ^ r3_reg[22]);

endmodule
