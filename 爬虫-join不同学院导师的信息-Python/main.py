# -*- coding: utf-8 -*-
"""
Created on Fri Mar 19 14:24:14 2021

Updated on Tue Jun 13 20:20:53 2023

@author: liurh
"""
import requests
import pandas as pd
from selenium import webdriver
from bs4 import BeautifulSoup
import numpy as np
import sys
from selenium.webdriver.chrome.options import Options
import logging

# 设置日志格式和等级
logging.basicConfig(filename='./info.log',
                    format='%(asctime)s-%(levelname)s-%(message)s',
                    level=logging.INFO)

# 定义URL和浏览器驱动路径
MAIN_URL = 'https://computer.hdu.edu.cn'
HDU_JOIN_URL = 'https://joint.hdu.edu.cn/9207/list.htm'
CHROME_DRIVER_PATH = 'D:/code/selenium/chromedriver.exe'


def parser(pro_url):
    """解析教师个人主页，提取姓名和其他信息"""
    data = requests.get(pro_url)
    data.encoding = 'utf-8'
    soup = BeautifulSoup(data.text, 'html.parser')
    name = soup.select('h1.arti_title')[0].text
    others = soup.select('div.wp_articlecontent p')
    others = [s.text for s in others]
    others = ''.join(others)

    return name, others


def muti_pages(main_url, num=1):
    """爬取计算机学院教师列表页，提取教师姓名和个人主页链接"""
    data = requests.get(f'{main_url}/6770/list{num}.htm')
    data.encoding = 'utf-8'
    soup = BeautifulSoup(data.text, 'html.parser')
    res = soup.select('div.uk-width-expand span.Article_MicroImage')
    t_urls = list()
    names, info = list(), list()
    for item in res:
        t_urls.append(f"{main_url}/{item.select('a')[0]['href']}")

    for t_url in t_urls:
        name, others = parser(t_url)
        names.append(name)
        info.append(others)
        # info.extend(others)
    return dict(zip(names, info))


def muti_run(main_url, start=1, end=7):
    """爬取计算机学院多个列表页，合并教师信息"""
    res = dict()
    for i in range(start, end):
        msg = f'Processing {i}/{end-1} ...'
        sys.stdout.write('\r' + msg)
        tmp = muti_pages(main_url, i)
        res.update(tmp)
    return res


def muti_parser_itmo():
    """爬取ITMO-Joint教师列表页，提取教师姓名和个人主页链接"""
    chrome_options = Options()
    chrome_options.add_argument('--headless')
    driver = webdriver.Chrome(CHROME_DRIVER_PATH, chrome_options=chrome_options)
    driver.get(HDU_JOIN_URL)
    html = driver.page_source
    soup = BeautifulSoup(html, 'html.parser')
    res = soup.select('div.gallery-card')
    names, urls = list(), list()
    tar = dict()
    for item in res[:40]:
        names.append(item.select('a')[0]['title'].strip())
        urls.append(item.select('a')[0]['href'].strip())
    for url in urls:
        tar.update(parser_itmo(url))
    return tar


def parser_itmo(url):
    """解析ITMO-Joint教师个人主页，提取姓名和其他信息"""
    data = requests.get(url)
    data.encoding = 'utf-8'
    values = list()
    soup = BeautifulSoup(data.text, 'html.parser')
    name = soup.select('h3 strong')[0].text
    values.append(name)
    values.append(soup.select('h5 span.badge')[0].text)
    values.append(soup.select('td p')[0].text)
    tmp = soup.select('div.alignP p')
    values.append(tmp[0].text)
    values.append(tmp[1].text)
    tmp = soup.select('ul.row span.col-sm-9')
    values.append(tmp[0].text)
    values.append(tmp[1].text)
    values.append(tmp[2].text)
    return {name: values}


def save_to_excel(data, file_name):
    """将数据保存到Excel文件中"""
    df = pd.DataFrame(data)
    # 设置列名
    df.columns = ['姓名', '职称', '称号', '专业', '研究方向',
                  'E-mail', '办公地址', '电话', '其他']
    # 保存到Excel文件中
    df.to_excel(file_name, index=False)
    return

def main():
    logging.info('开始爬取计算机学院教师信息')
    computer = muti_run(MAIN_URL)
    logging.info('完成爬取计算机学院教师信息')
    
    logging.info('开始爬取ITMO学院教师信息')
    itmo = muti_parser_itmo()
    logging.info('完成爬取ITMO学院教师信息')
    
    drop = list()
    arr = list()
    
    logging.info('开始合并教师信息')
    
    for item in itmo:
    
        tmp = itmo[item]
        if c_data := computer.get(item):
            tmp.append(c_data)
            arr.append(tmp)
        else:
            drop.append(item)
        itmo[item] = tmp
    
    [itmo.pop(k) for k in drop]
    
    narr = np.array(arr, dtype='U1024')
    
    # 转换为DataFrame
    data_df = pd.DataFrame(narr)
    
    save_to_excel(data_df, 'F:/teacher.xlsx')
    
    logging.info('完成合并教师信息')
    
    logging.info('程序运行结束')
    return

main()
