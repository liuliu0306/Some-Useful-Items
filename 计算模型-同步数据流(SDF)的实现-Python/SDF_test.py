# -*- coding: utf-8 -*-
"""
Created on Fri Jun 10 10:35:53 2022

@author: liurh
"""

import unittest
from SDF import SDF, Node, arg_type


class SDFTest(unittest.TestCase):
    def test_execute(self):
        # Test_execute provides two SDF cases and
        # verifies the correctness of the final result.
        case1 = SDF('case1')

        case1.add_node('root1', lambda x: None)
        case1.add_node('root2', lambda x: None)
        case1.add_node('end', lambda x: None)
        case1.add_node('A_square', lambda x: x**2)
        case1.add_node('B_square',lambda x: x**2)
        case1.add_node('C_add',lambda x, y: x+y)

        case1.add_token('root1', 'A_square', [1,2,3])
        case1.add_token('root2', 'B_square', [4,5,6])
        case1.add_token('A_square', 'C_add', [])
        case1.add_token('B_square', 'C_add', [])
        case1.add_token('C_add', 'end', [])

        case1.execute()

        # Finall result is [1,2,3]**2 + [4,5,6]**2 = [17,29,45]
        for inx, edge in enumerate(case1.token_vec):
            if 'end' in edge[1]:
                self.assertEqual(edge[2], [17,29,45])

        case2 = SDF('case2')
        case2.add_node('root1', lambda x: None)
        case2.add_node('end', lambda x: None)
        case2.add_node('A_cube', lambda x: x**3)
        
        case2.add_token('root1', 'A_cube', [2,3,4])
        case2.add_token('A_cube', 'end', [])
        case2.execute()
        
        # Finall result is [2,3,4]**3 = [8,27,64]
        for inx, edge in enumerate(case2.token_vec):
            if 'end' in edge[1]:
                self.assertEqual(edge[2], [8,27,64])

    def test_add_node(self):
        sdf = SDF('add_node')
        sdf.add_node('node1', lambda x: None)
        self.assertEqual(sdf.nodes.pop().name, 'node1')

    def test_add_token(self):
        sdf = SDF('add_token')
        sdf.add_node('node1', lambda x: None)
        sdf.add_node('node2', lambda x: None)
        sdf.add_token('node1', 'node2', [])
        self.assertEqual(sdf.token_vec, [['node1', 'node2', []]])

    def test_if_fire(self):
        sdf = SDF('_if_fire')
        sdf.add_node('node1', lambda x: x)
        sdf.add_node('node2', lambda x: x)
        sdf.add_token('node1', 'node2', [1, 2, 3])
        self.assertEqual(sdf._if_fire('node1'), False)
        self.assertEqual(sdf._if_fire('node2'), True)

    def test_update_fire(self):
        sdf = SDF('_update_fire')
        sdf.add_node('node1', lambda x: x)
        sdf.add_node('node2', lambda x: x)
        sdf.add_token('node1', 'node2', [1, 2, 3])
        sdf._update_fire()
        self.assertEqual(sdf.if_fire_vec, [False, True])

    def test_get_data_from_token_vec(self):
        sdf = SDF('_update_fire')
        sdf.add_node('node1', lambda x: x)
        sdf.add_node('node2', lambda x: x)
        sdf.add_token('node1', 'node2', [1, 2, 3])
        data = sdf._get_data_from_token_vec(sdf.nodes[1])
        self.assertEqual(data, [1])

    def test_update_token_vec(self):
        sdf = SDF('_update_fire')
        sdf.add_node('node1', lambda x: x)
        sdf.add_node('node2', lambda x: x)
        sdf.add_token('node1', 'node2', [1, 2, 3])
        sdf._update_token_vec(sdf.nodes[0], 4)
        self.assertEqual(sdf.token_vec, \
                         [['node1', 'node2', [1, 2, 3, 4]]])


class NodeTest(unittest.TestCase):
    def test_Node(self):
        operator = lambda x: x**2
        name = 'test_node_1'
        node = Node(name, operator)
        self.assertEqual(node.name, name)
        self.assertEqual(node.operator, operator)

    def test_operator(self):
        operator = lambda x: x**2
        name = 'test_node_2'
        node = Node(name, operator)
        _input = 5
        _output = node.operator(_input)
        self.assertEqual(25, _output)


if __name__ == '__main__':
    unittest.main()