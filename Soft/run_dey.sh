mkdir -p T3_out
bin/data_process_sngl/dey_bhattacharya_frery_lopez_rao_4components_decomposition_FP.exe -id T3 -od T3_out -fnr 624 -fnc 1920 -nwr 1 -nwc 1 -ofr 0 -ofc 0 -iodf T3
cd T3_out
cat Dey_FP_Dbl.bin Dey_FP_Vol.bin Dey_FP_Odd.bin Dey_FP_Hlx.bin Dey_FP_Theta.bin Dey_FP_Tau.bin > stack.bin
cd ..
