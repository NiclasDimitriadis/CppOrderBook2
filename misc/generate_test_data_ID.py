import numpy as np
import pandas as pd


#generate random walk for price simulation
#repeat if negative prices are generated
n = 1000000
n_withdrawels = 0
withdrawel_IDs = []
msg_id = 0
rand_walk = np.zeros(n) 
while(rand_walk.min() < 500 or rand_walk.max() > 1500):
    rand_walk = np.random.randint(low=-1, high=2, size=n)
    rand_walk[0] = 1000
    rand_walk = np.cumsum(rand_walk)
    
#make pandas series for all columns
order_ID_series = pd.Series(range(n))
order_type_series = pd.Series(np.random.choice(3,n,p=[.48,.04,.48])) + 3
order_volume_series = pd.Series(np.random.randint(low=-3000, high=3000, size=n))
#add 1 whereever volume is zero
order_volume_series = order_volume_series + 1 * pd.Series(order_volume_series == 0)
order_price_series_rw = pd.Series(rand_walk)

#33 as 'fake' ID, necessary to avoid invalid index errors if a withdraw msg is before the first limit orders msg  
limit_order_IDs = [33]
order_withdraw_ID_series = pd.Series(np.zeros(n))


for i in range(n):
    order_type = order_type_series[i]
    order_ID = order_ID_series[i]
    
    if (order_type == 3):
        limit_order_IDs.append(order_ID)
        
    n_limit_orders = len(limit_order_IDs)
    draw_from = min(n_limit_orders, 100)
    withdraw_ID_index = n_limit_orders - 1 - np.random.randint(draw_from)
    withdraw_ID = limit_order_IDs[withdraw_ID_index]
    order_withdraw_ID_series[i] = withdraw_ID
    
    #generate valid withdraw volume for withdraw msg
    if (order_type == 4):
        volume = 0
        if (withdraw_ID != 33):
            max_vol = order_volume_series[withdraw_ID]
            mult = 1 - 2 * (max_vol < 0)
            #print(max_vol)
            rand_withdraw_vol = np.random.randint(abs(max_vol)) * mult
            draw_list = [max_vol, rand_withdraw_vol]
            volume = draw_list[np.random.choice(2, p=[.9,.1])]
        order_volume_series[i] = volume
            

#make dictionary of series for creating dataframes
frame_dict_rw = {
    'order_ID': order_ID_series,
    'order_type': order_type_series,
    'order_volume': order_volume_series,
    'order_price': order_price_series_rw,
    'order_withdraw_ID': order_withdraw_ID_series
    }

#make dataframes from dictionaries
csv_frame_rw = pd.DataFrame(frame_dict_rw)

#generate CSVs
csv_frame_rw.to_csv('RandTestDataRW.csv', header=False, index=False)
