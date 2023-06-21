# -*- coding: utf-8 -*-
"""
Created on Wed Jun  8 19:32:48 2022

@author: liurh
"""

import types
import numpy as np
import graphviz
from typing import Union
import logging
import os
import inspect

filename = inspect.getframeinfo(inspect.currentframe()).filename
path = os.path.dirname(os.path.abspath(filename))
os.chdir(path)

logging.basicConfig(filename='./log/SDF.log',
    format = '%(asctime)s-%(levelname)s-%(funcName)s:\n%(message)s\n',
    level=logging.INFO)

def arg_type(arg_index: Union[int, list], arg_type):
    # Check function arguments and values.
    def trace(f):
        def traced(*args):
            for ind, t in zip(arg_index, arg_type):
                if ind == len(args):
                    break
                assert isinstance(args[ind], t), \
                    'The type of argument {} is not {}'.format(args[ind], t)
            return f(*args)
        return traced
    return trace


# Synchronous Data Flow class
class SDF():
    """
    Parameters:
    1. name -> Name of Synchronous Data Flow model.
    2. nodes -> Nodes of Synchronous Data Flow model, see class Node.
    3. if_fire_vec -> Activation vector, which is a vector that
                    records the activation state of all nodes, 
                    the activation indicates that it can be fired.
    4. token_vec -> This is a list of triples whose shape is:
                [start node of edge, end point of edge, token].
                This vector is updated every time when the token is updated.

    The main function is the execute function, 
    which controls the update of the token and the flow of data.
    """
    @arg_type([1, 2, 3, 4], [str, list, list, list])
    def __init__(self, name):
        self.name = name
        self.nodes = []
        self.if_fire_vec = []
        self.token_vec = []

    @arg_type([1, 2], [str, types.FunctionType])
    def add_node(self, name, operator):
        for node in self.nodes:
            assert name != node.name, f'Node {name} is areadly exist.'
        node = Node(name, operator)
        self.nodes.append(node)

    @arg_type([1, 2, 3], [str, str, list])
    def add_token(self, edge_in, edge_out, data):
        for edge in self.token_vec:
            assert edge != [edge_in, edge_out, data], \
                f'Token {[edge_in, edge_out, data]} is areadly exist.'
        self.token_vec.append([edge_in, edge_out, data])

    @arg_type([1], [str])
    def _if_fire(self, current_node):
        # Check if a node can fire
        if 'end' in current_node:
            return False
        numpy_vec = np.array(self.token_vec, dtype=object)
        tmp = np.where(numpy_vec[:,1] == current_node)[0]
        # tmp is the index array of a col which name is current_node.
        if len(tmp) > 0:
            for token in numpy_vec[tmp, 2]:
                if token == []:
                    return False
            return True
        else:
            return False

    def _update_fire(self):
        # Update the state of one node.
        self.if_fire_vec = [False for i in range(len(self.nodes))]
        for num, node in enumerate(self.nodes):
            if self._if_fire(node.name):
                self.if_fire_vec[num] = True

    @arg_type([1], [object])
    def _get_data_from_token_vec(self, node):
        # Get data from each node's token.
        buffer = []
        numpy_vec = np.array(self.token_vec, dtype=object)
        tmp = np.where(numpy_vec[:,1] == node.name)[0]
        for inx in tmp:
            buffer.append(self.token_vec[inx][2].pop(0))
        return buffer

    @arg_type([1, 2], [object, int])
    def _update_token_vec(self, node, token):
        # Scan the token_vec vector to 
        # update the activation state of each node.
        numpy_vec = np.array(self.token_vec, dtype=object)
        tmp = np.where(numpy_vec[:,0] == node.name)[0]
        for inx in tmp:
            self.token_vec[inx][2].append(token)

    def execute(self):
        # Run the network, core function.
        """
        Each round loops for each node: 
        update activation status -> get tokens -> 
        perform node operations -> update activation status
        """
        epoch = 0
        self.visualize(epoch)
        self._update_fire()
        target = [False for i in range(len(self.nodes))]
        while target != self.if_fire_vec:
            epoch += 1
            logging.info(f'Epoch-{epoch}')
            for ni, fire in enumerate(self.if_fire_vec):
                if fire == True:

                    node = self.nodes[ni]
                    logging.info(f'Update Node:{node.name}')
                    # print(node.name)
                    buffer = self._get_data_from_token_vec(node)
                    token = node.operator(*buffer)
                    self._update_token_vec(node, token)
                    self.if_fire_vec[ni] = False
                    self._update_fire()
                    self.visualize(epoch)
                    logging.info(f'Current token_vec:\n{self.token_vec}\n')
                    # print(self.token_vec)

    @arg_type([1], [str])
    def show(self, path):
        with open(path) as f:
            dot_graph = f.read()
        dot = graphviz.Source(dot_graph)
        dot.render(filename=f'{self.name}-epoch{path[-5:-4]}',\
                   directory='./figure', view=False)
        # dot.view()

    @arg_type([1], [int])
    def visualize(self, epoch):
        res = ['\n']
        res.append('digraph G {')
        res.append(' rankdir=LR;')
        nodes_name = []

        # Plot nodes
        for i, node in enumerate(self.nodes):
            if 'root' in node.name:
                res.append(f' {node.name}[shape=rarrow];')
                res.append(f' {node.name} -> n_{i};')
            if 'end' in node.name:
                res.append(f' {node.name}[shape=rarrow];')
                res.append(f' n_{i} -> {node.name};')
        for i, node in enumerate(self.nodes):
            res.append(f' n_{i}[label="{node.name}"];')
            nodes_name.append(node.name)

        # Plot edges
        for i, edge in enumerate(self.token_vec):
            _from = nodes_name.index(edge[0])
            _to = nodes_name.index(edge[1])
            res.append(f' n_{_from} -> n_{_to}[label="{edge[2]}"];')

        res.append("}")
        path = f'./figure/{self.name}-{epoch}.dot'
        f = open(path, 'w')
        f.write('\n'.join(res))
        f.close()
        self.show(path)
        

class Node(object):
    @arg_type([1, 2], [str, types.FunctionType])
    def __init__(self, name, operator):
        self.name = name
        self.operator = operator

    # Firing function
    @arg_type([1], [list])
    def firing(self, input):
        return self.operator(*input)


if __name__ == '__main__':

    hello = SDF('hello')
    
    hello.add_node('root1', lambda x:None)
    hello.add_node('root2', lambda x:None)
    hello.add_node('end', lambda x:None)
    hello.add_node('A_square', lambda x:x**2)
    hello.add_node('B_square',lambda x:x**2)
    hello.add_node('C_add',lambda x, y:x+y)
    
    hello.add_token('root1', 'A_square', [1,2,3])
    hello.add_token('root2', 'B_square', [4,5,6])
    hello.add_token('A_square', 'C_add', [])
    hello.add_token('B_square', 'C_add', [])
    hello.add_token('C_add', 'end', [])
    
    hello.execute()
