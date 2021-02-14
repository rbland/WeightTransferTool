# Overview
A C++ plugin for Maya to manage vertex attribute transfer.

A typical production pipeline often requires topology changes to be made to an asset after texturing and rigging have begun. This means that any vertex attributes that were assigned (such as joint weights and UVs) must be redone. To address this issue this transfer tool works by iterating through the destination model's vertices and sampling vertex attributes at the closest position on the source model surface.