//
// Copyright (C) 2016 OpenSim Ltd.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program; if not, see <http://www.gnu.org/licenses/>.
//

package inet.visualizer.networknode;

import inet.visualizer.base.NetworkConnectionVisualizerBase;
import inet.visualizer.contract.INetworkConnectionVisualizer;

//
// This module visualizes multiple network connections on a 2D canvas.
//
// @see ~NetworkConnectionCanvasVisualizer, ~NetworkConnectionVisualizer,
// ~NetworkConnectionVisualizerBase, and ~INetworkConnectionVisualizer
//
simple NetworkConnectionCanvasVisualizer extends NetworkConnectionVisualizerBase like INetworkConnectionVisualizer
{
    parameters:
        networkNodePathFilter = default(""); // disabled because omnetpp displays network connections by default 
        @class(NetworkConnectionCanvasVisualizer);
}
