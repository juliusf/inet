//
// Copyright (C) OpenSim Ltd.
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

#ifndef __INET_LINKBREAKVISUALIZERBASE_H
#define __INET_LINKBREAKVISUALIZERBASE_H

#include "inet/linklayer/common/MACAddress.h"
#include "inet/visualizer/base/VisualizerBase.h"
#include "inet/visualizer/common/AnimationPosition.h"
#include "inet/visualizer/common/InterfaceFilter.h"
#include "inet/visualizer/common/LineManager.h"
#include "inet/visualizer/common/NetworkNodeFilter.h"
#include "inet/visualizer/common/PacketFilter.h"

namespace inet {

namespace visualizer {

class INET_API LinkBreakVisualizerBase : public VisualizerBase, public cListener
{
  protected:
    class INET_API LinkBreakVisualization {
      public:
        mutable AnimationPosition linkBreakAnimationPosition;
        const int transmitterModuleId = -1;
        const int receiverModuleId = -1;

      public:
        LinkBreakVisualization(int transmitterModuleId, int receiverModuleId);
        virtual ~LinkBreakVisualization() {}
    };

  protected:
    /** @name Parameters */
    //@{
    cModule *subscriptionModule = nullptr;
    bool displayLinkBreaks = false;
    NetworkNodeFilter nodeFilter;
    InterfaceFilter interfaceFilter;
    PacketFilter packetFilter;
    const char *icon = nullptr;
    double iconTintAmount = NaN;
    cFigure::Color iconTintColor;
    const char *fadeOutMode = nullptr;
    double fadeOutTime = NaN;
    double fadeOutAnimationSpeed = NaN;
    //@}

    std::map<std::pair<int, int>, const LinkBreakVisualization *> linkBreakVisualizations;

  protected:
    virtual void initialize(int stage) override;
    virtual void refreshDisplay() const override;

    virtual void subscribe();
    virtual void unsubscribe();

    virtual const LinkBreakVisualization *createLinkBreakVisualization(cModule *transmitter, cModule *receiver) const = 0;
    virtual void addLinkBreakVisualization(const LinkBreakVisualization *linkBreakVisualization);
    virtual void removeLinkBreakVisualization(const LinkBreakVisualization *linkBreakVisualization);
    virtual void setAlpha(const LinkBreakVisualization *linkBreakVisualization, double alpha) const = 0;

    virtual cModule *findNode(MACAddress address);

  public:
    virtual ~LinkBreakVisualizerBase();

    virtual void receiveSignal(cComponent *source, simsignal_t signal, cObject *object, cObject *details) override;
};

} // namespace visualizer

} // namespace inet

#endif // ifndef __INET_LINKBREAKVISUALIZERBASE_H

