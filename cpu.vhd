library ieee;
use ieee.std_logic_1164.all;
use ieee.std_logic_arith.all;



entity cpu  is
  port (  read_data_bus:in std_logic_vector(16 downto 1):=X"0000";
          write_data_bus:out std_logic_vector(16 downto 1):=X"0000";
          addr_bus:out std_logic_vector(16 downto	 1):=X"0000";
          RW:out std_logic:='0';
          SIXTEEN_BIT:out std_logic:='0';
          ADDR_RDY:out std_logic:='0';
          DATA_RDY_N:in std_logic;
          MEM_HOLD:in std_logic;
          DOT_SEGMENT:out std_logic:='0';          
          clock: in std_logic );         
end entity cpu;



architecture behaviour of cpu is
	type CpuStates is ( Ins_Fetch,Ins_Decode,Ins_Execute,Ins_Print,
	                    Wait_For_MemRdyHigh,Wait_For_MemRdyLow,Setup_MemCmd,Complete_MemCmd );
	                    
	signal cpu_state:CpuStates := Ins_Fetch;
	signal sys_clk :std_logic;
	type RegBank is array (0 to 15) of std_logic_vector(16 downto 1);
	signal reg_bank:RegBank;
	

				

begin  
    sys_clk <= (clock and  (not MEM_HOLD)) ;
   	
    cpu_sm: process is
    
    variable read_data:  std_logic_vector(16 downto 1):=X"0000"; 
    variable write_data: std_logic_vector(16 downto 1):=X"0000"; 
    variable address:    std_logic_vector(16 downto 1):=X"0000";
    variable sig_rw: std_logic:='0';
    variable sig_sixteen_bit: std_logic:='0';
    variable pc: natural := 0;
	variable prev_cpu_state: CpuStates :=  cpu_state; 
	variable instruction: std_logic_vector(16 downto 1) := X"0000";
	alias opcode is instruction(16 downto 12);
	alias reg1 is instruction(11 downto 8);
	alias reg2 is instruction(7 downto 4);
	--only for slt
	alias slt_reg2 is instruction(8 downto 5);
	alias slt_reg3 is instruction(4 downto 1);
	alias reg1_imm is instruction(11 downto 9);
	variable temp_int:integer;
	variable temp_int2:integer;
	variable temp_reg:std_logic_vector(16 downto 1) := X"0000";
	alias LR is reg_bank(13);
	
    
    begin
    
    wait until rising_edge(sys_clk);
    
    case cpu_state is 
		
    when Ins_Fetch =>
    
    address := conv_std_logic_vector(pc, 16);
    --addr_bus <= conv_std_logic_vector(pc, 16);
    sig_rw := '0';
    RW<='0';
    sig_sixteen_bit := '1';
    
    prev_cpu_state := cpu_state;
    cpu_state <= Wait_For_MemRdyHigh;    
    
    when Ins_Print=>
    
	instruction(16 downto 9) := read_data(8 downto 1);
	instruction(8 downto 1) := read_data(16 downto 9);
	
	address := X"FF02";
	addr_bus <= X"FF02";
    sig_rw := '1';
    RW <='1';
    sig_sixteen_bit := '1';
    write_data := X"0000";
    write_data(16 downto 12) := opcode;
    
    prev_cpu_state := cpu_state;
    cpu_state <= Wait_For_MemRdyHigh;		
    
    when Ins_Decode =>
    -- look at opcode
    -- collect operands 
    
    instruction(16 downto 9) := read_data(8 downto 1);
	instruction(8 downto 1) := read_data(16 downto 9); 
       
    
    if(opcode = B"10011") then
		--li
		
		temp_int := conv_integer(unsigned(reg1_imm));
		reg_bank(temp_int) <= X"0000";
		reg_bank(temp_int) <= instruction(8 downto 1) or X"0000";
		
		prev_cpu_state := cpu_state;
		cpu_state <= Ins_Execute;
	
	elsif(opcode = B"01101") then	
		--rs
		temp_int := conv_integer(unsigned(instruction(7 downto 1)));
		
		for i in 1 to 16 loop						
		  if (i < (16-temp_int)) then
			reg_bank(conv_integer(unsigned(reg1)))(i) <= reg_bank(conv_integer(unsigned(reg1)))(i+temp_int);			
		  else
			reg_bank(conv_integer(unsigned(reg1)))(i) <= '0';
		  end if;
			
		end loop;
		
		prev_cpu_state := cpu_state;
		cpu_state <= Ins_Execute;
		
	elsif(opcode = B"01100") then		
		--ls	
						
		temp_int := conv_integer(unsigned(instruction(7 downto 1)));
		
		for i in 16 downto 1 loop						
		  if (i > temp_int) then
			reg_bank(conv_integer(unsigned(reg1)))(i) <= reg_bank(conv_integer(unsigned(reg1)))(i-temp_int);		
		  else
			reg_bank(conv_integer(unsigned(reg1)))(i) <= '0';
		  end if;
			
		end loop;
		
		prev_cpu_state := cpu_state;
		cpu_state <= Ins_Execute;
		
	elsif(opcode = B"01010") then
		--or
		reg_bank(conv_integer(unsigned(reg1))) <= reg_bank(conv_integer(unsigned(reg1))) or												  reg_bank(conv_integer(unsigned(reg2)));
		
		prev_cpu_state := cpu_state;
		cpu_state <= Ins_Execute;
	
	elsif(opcode = B"01000") then
		--and
		reg_bank(conv_integer(unsigned(reg1))) <= reg_bank(conv_integer(unsigned(reg1))) and
												  reg_bank(conv_integer(unsigned(reg2)));
		
		prev_cpu_state := cpu_state;
		cpu_state <= Ins_Execute;

	elsif(opcode = B"00110") then
		--sub
		temp_int := conv_integer(signed(reg_bank(conv_integer(unsigned(reg1)))));				
		temp_int2 := conv_integer(signed(reg_bank(conv_integer(unsigned(reg2)))));
		temp_reg := conv_std_logic_vector(temp_int - temp_int2,16);		
		
		reg_bank(conv_integer(unsigned(reg1))) <= temp_reg;
		
		
		prev_cpu_state := cpu_state;
		cpu_state <= Ins_Execute;

	elsif(opcode = B"00100") then	
		--add
		temp_int := conv_integer(signed(reg_bank(conv_integer(unsigned(reg1)))));				
		temp_int2 := conv_integer(signed(reg_bank(conv_integer(unsigned(reg2)))));
		temp_reg := conv_std_logic_vector(temp_int + temp_int2,16);		
		
		reg_bank(conv_integer(unsigned(reg1))) <= temp_reg;
		
		
		prev_cpu_state := cpu_state;
		cpu_state <= Ins_Execute;
	
	elsif(opcode = B"00111") then	
		--subi
		temp_int:= conv_integer(signed(reg_bank(conv_integer(unsigned(reg1_imm)))));
		temp_int2 := conv_integer(signed(instruction(8 downto 1)));
		temp_int:= temp_int - temp_int2 ;
		reg_bank(conv_integer(unsigned(reg1_imm))) <= conv_std_logic_vector(temp_int,16);		
		
		prev_cpu_state := cpu_state;
		cpu_state <= Ins_Execute;
		
	elsif(opcode = B"00101") then		
		--addi
		temp_int:= conv_integer(signed(reg_bank(conv_integer(unsigned(reg1_imm)))));
		temp_int := temp_int + conv_integer(signed(instruction(8 downto 1)));
		reg_bank(conv_integer(unsigned(reg1_imm))) <= conv_std_logic_vector(temp_int,16);		
		
		prev_cpu_state := cpu_state;
		cpu_state <= Ins_Execute;
		
	elsif(opcode = B"01001") then
		--andi
		temp_reg := X"FF00" or instruction(8 downto 1);
		reg_bank(conv_integer(unsigned(reg1_imm))) <=  reg_bank(conv_integer(unsigned(reg1_imm))) and temp_reg;
		
				
		prev_cpu_state := cpu_state;
		cpu_state <= Ins_Execute;
		
	
	elsif(opcode = B"10010") then
		--slt
		if(conv_integer(signed(reg_bank(conv_integer(unsigned(slt_reg2))))) < conv_integer(signed(reg_bank(conv_integer(unsigned(slt_reg3)))))) then
			reg_bank(conv_integer(unsigned(reg1_imm))) <= X"0001";			
		else
			reg_bank(conv_integer(unsigned(reg1_imm))) <= X"0000";
		end if;	
		
		
		prev_cpu_state := cpu_state;
		cpu_state <= Ins_Execute;
		
	elsif(opcode = B"10101") then
		--comp
		reg_bank(conv_integer(unsigned(reg1))) <= conv_std_logic_vector(0-conv_integer(signed(reg_bank(conv_integer(unsigned(reg1))))),16);
		
		prev_cpu_state := cpu_state;
		cpu_state <= Ins_Execute;	
	
		
	elsif(opcode = B"10001") then	
		--mov
		reg_bank(conv_integer(unsigned(reg1))) <=  reg_bank(conv_integer(unsigned(reg2)));
		
		prev_cpu_state := cpu_state;
		cpu_state <= Ins_Execute;
		
	elsif(opcode = B"01011") then		
		--ori
		reg_bank(conv_integer(unsigned(reg1_imm))) <= instruction(8 downto 1) or reg_bank(conv_integer(unsigned(reg1_imm)));		
		
		prev_cpu_state := cpu_state;
		cpu_state <= Ins_Execute;
		
	elsif(opcode = B"00000") then
		--lw
		temp_int := conv_integer(unsigned(instruction(3 downto 1)));
		temp_int := temp_int + conv_integer(unsigned(reg_bank(conv_integer(unsigned(instruction(7 downto 4))))));
		address := conv_std_logic_vector(temp_int,16);
		--addr_bus <= conv_std_logic_vector(temp_int,16);		
		sig_rw := '0';
		RW <= '0';
		sig_sixteen_bit := '1';
    
		prev_cpu_state := cpu_state;
		cpu_state <= Wait_For_MemRdyHigh;
	elsif(opcode = B"00001") then
		--sw
		temp_int := conv_integer(unsigned(instruction(3 downto 1)));
		temp_int := temp_int + conv_integer(unsigned(reg_bank(conv_integer(unsigned(instruction(7 downto 4))))));
		address := conv_std_logic_vector(temp_int,16);
		--addr_bus <= conv_std_logic_vector(temp_int,16);
		sig_rw := '1';
		RW <= '1';
		sig_sixteen_bit := '1';	
		
		write_data := reg_bank(conv_integer(unsigned(reg1)));		
    
		prev_cpu_state := cpu_state;
		cpu_state <= Wait_For_MemRdyHigh;		

	elsif(opcode = B"00010") then
		--lb
		
		temp_int := conv_integer(unsigned(instruction(3 downto 1)));
		temp_int := temp_int + conv_integer(unsigned(reg_bank(conv_integer(unsigned(instruction(7 downto 4))))));
		address := conv_std_logic_vector(temp_int,16);
		--addr_bus <= conv_std_logic_vector(temp_int,16);		
		sig_rw := '0';
		RW <= '0';
		sig_sixteen_bit := '0';
    
		prev_cpu_state := cpu_state;
		cpu_state <= Wait_For_MemRdyHigh;	
		
	elsif(opcode = B"00011") then
		--sb
		
		temp_int := conv_integer(unsigned(instruction(3 downto 1)));
		temp_int := temp_int + conv_integer(unsigned(reg_bank(conv_integer(unsigned(instruction(7 downto 4))))));
		address := conv_std_logic_vector(temp_int,16);
		--addr_bus <= conv_std_logic_vector(temp_int,16);
		sig_rw := '1';
		RW <= '1';
		sig_sixteen_bit := '0';		
		
		if(address(1) = '1') then
		--odd address.Hence higher byte gets assigned
		write_data(8 downto 1) := reg_bank(conv_integer(unsigned(reg1)))(16 downto 9);				
		else
		--even address.Hence lower byte gets assigned		
		write_data(8 downto 1) := reg_bank(conv_integer(unsigned(reg1)))(8 downto 1);
		end if;
    
		prev_cpu_state := cpu_state;
		cpu_state <= Wait_For_MemRdyHigh;				
		
	else
		prev_cpu_state := cpu_state;
		cpu_state <= Ins_Execute;
	
	end if;		
    
    when Ins_Execute=>          
         
    pc:=pc + 2;
    
    if(opcode = B"11000") then	
			pc:=0;	
	elsif(opcode = B"00010") then
		--lb				
		if(address(1) = '1') then
		--odd address.Hence higher byte gets assigned		
		reg_bank(conv_integer(unsigned(reg1)))(16 downto 9) <= read_data(8 downto 1);
		else
		--even address.Hence lower byte gets assigned		
		reg_bank(conv_integer(unsigned(reg1)))(8 downto 1) <= read_data(8 downto 1);
		end if;
		
	elsif(opcode = B"01111") then	
		--beq
		--pc is already incremented by 2.take that into account.
		if( reg_bank(conv_integer(unsigned(reg1))) = reg_bank(conv_integer(unsigned(reg2))) ) then
			pc:=pc+0;
		else
			pc:= pc+2;
		end if;

	elsif(opcode = B"01110") then
		--bneq
		
		--pc is already incremented by 2.take that into account.
		if( reg_bank(conv_integer(unsigned(reg1))) = reg_bank(conv_integer(unsigned(reg2))) ) then
			pc:=pc+2;
		else
			pc:= pc+0;
		end if;
		
	elsif(opcode = B"00000") then
		--lw
		reg_bank(conv_integer(unsigned(reg1))) <= read_data;
		
	elsif(opcode = B"10100") then
		--bal
		-- store the next instruction address in LR
		LR <= conv_std_logic_vector(pc,16);
		--pc was added with 2 above.undo that.
		pc:=pc-2;		
		pc := pc + conv_integer(signed(instruction(11 downto 1)));
		
	
	elsif(opcode = B"10110") then	
		--ret
		pc := conv_integer(unsigned(LR));
		
	elsif(opcode = B"10111") then
		--b
		
		--pc was added with 2 above.undo that.
		pc:=pc-2;				
		pc := pc + conv_integer(signed(instruction(11 downto 1)));
	
			
	end if;    
    
    prev_cpu_state := cpu_state;
    cpu_state <= Ins_Fetch;
    
        
    
---------------------------------------------------------------------------------
---------------------------------------------------------------------------------
--Memory State Machine
    
    when  Wait_For_MemRdyHigh =>
		if DATA_RDY_N = '0' then
			cpu_state <= Wait_For_MemRdyHigh;
		else
			cpu_state <= Setup_MemCmd;
		end if;
        
    when  Setup_MemCmd =>
         --RW <= sig_rw;
         SIXTEEN_BIT <=  sig_sixteen_bit;
         addr_bus <= address;   
         ADDR_RDY <= '1';
         cpu_state <= Wait_For_MemRdyLow;
         
         
    when Wait_For_MemRdyLow =>
		
		if DATA_RDY_N = '1' then
			cpu_state <= Wait_For_MemRdyLow;
		else
			cpu_state <= Complete_MemCmd;
		end if;
        
    when Complete_MemCmd =>
		if(sig_rw = '1') then
			write_data_bus <= write_data;
		elsif(sig_rw = '0') then					
			read_data := read_data_bus;
		end if;  
		ADDR_RDY <= '0';    
		
		-- go to next state depending 
		-- on how we got here
		if (   prev_cpu_state =  Ins_Fetch ) then
				cpu_state <= Ins_Decode;				
		elsif(   prev_cpu_state =  Ins_Print ) then
				cpu_state <= Ins_Decode;	
		elsif(   prev_cpu_state =  Ins_Decode ) then
				cpu_state <= Ins_Execute;				
		elsif(   prev_cpu_state =  Ins_Execute ) then
				cpu_state <= Ins_Fetch;
		end if;   
		
		
		end case;   
           
    end process cpu_sm;   
    
    
end behaviour;