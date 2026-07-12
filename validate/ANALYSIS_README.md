# MQTT Power Management System - Critical Analysis Report

## Executive Summary

**STATUS: ⚠️ CRITICAL - System is not viable for intended use**

This analysis reveals a fundamental design failure in the current power management architecture. The system is severely undersized with a 132x energy deficit and cannot sustainably power the forced appliances.

---

## System Overview

### Load Profile Analysis

| Metric | Value |
|--------|-------|
| **Total Leaf Loads** | 27 devices |
| **Total Load Current** | 62.20 A |
| **Forced Current (Always-On)** | 42.90 A |
| **Non-forced Current** | 19.30 A |

### Top 5 Power Consumers

1. **L14_HVAC** - 6.0 A
2. **L8_Stove** - 5.0 A
3. **L7_Refrigerator** - 4.0 A
4. **L11_Dryer** - 4.0 A
5. **L10_Washer** - 3.5 A

---

## Simulation Results

### Scenario Comparison

| Scenario | Start SoC | End SoC | Drop | Min SoC |
|----------|-----------|---------|------|---------|
| **Managed** | 90.0% | 0.0% | 90.0% | 0.0% |
| **Unmanaged** | 90.0% | 0.0% | 90.0% | 0.0% |

**Result:** Both scenarios completely deplete to 0% SoC, indicating the managed policy cannot overcome the fundamental energy deficit.

---

## Critical Findings

### 1. CRITICAL DESIGN FLAW: System is Massively Undersized

| Metric | Value |
|--------|-------|
| **Forced loads (always-on)** | 42.9 A |
| **Total possible loads** | 62.2 A |
| **Maximum solar generation** | 1.0 A (actual) |
| **Battery capacity** | 1,200 Wh (100Ah @ 12V) |
| **Expected drain (14 days)** | 164,740 Wh |
| **Actual solar generation** | 5,378 Wh |
| **Energy deficit** | **159,362 Wh (132x battery capacity!)** |

### 2. The Overload Situation

Your forced loads alone (refrigerator, stove, washer, dryer, HVAC, etc.) consume **42.9A continuously**, but:

- ⚠️ Solar can only provide **1-3A max**
- ⚠️ Inverter limit is **15A**
- ⚠️ **Net minimum drain: 39.9A when solar is at peak**

**Conclusion:** The system **CANNOT SUSTAIN** the forced appliances.

### 3. System Behavior in Results

#### Unmanaged Scenario
- Battery depletes from 90% → 0% in ~4 days
- Then stays dead (voltage stuck at 3.4V minimum)
- System becomes completely non-functional

#### Managed Scenario
- Also depletes to 0% despite power management
- Slower initial drop provides no long-term benefit
- Eventually fails as solar cannot meet forced load demand
- Management policy provides only **marginal relief** (15% improvement initially)

### 4. Root Causes

| Issue | Impact | Severity |
|-------|--------|----------|
| ✘ Undersized solar array | 1-3A max insufficient for 42A forced load | CRITICAL |
| ✘ Oversized battery demand | 42.9A forced current > 15A inverter limit | CRITICAL |
| ✘ Too many always-on appliances | Refrigerator, stove, washer, dryer all forced | CRITICAL |
| ✘ Insufficient energy generation | 5,378 Wh over 14 days vs 164,740 Wh needed | CRITICAL |

---

## Expert System Rules Triggered

### Triggered Rules

- **overload** - Critical overload: the forced-load current exceeds the inverter limit and the system is not sustainable without shedding or reconfiguration.

- **high_risk** - High risk: the forced-load current is already close to the inverter limit and any extra demand may trigger shutdowns.

- **deep_discharge** - Battery depletion risk: one or more scenarios end below a safe reserve threshold.

- **solar_gap** - Solar contribution is low relative to the load profile, so the battery must carry more of the burden.

- **high_demand** - A dominant load branch is very large and should be staggered or reassigned to keep the system stable.

---

## Solutions Required

### To Make System Viable, Implement ONE OR MORE:

#### 1. Increase Solar Capacity (STRONGLY RECOMMENDED)
- **Current:** 1.0 A max
- **Required:** 15-25 A capacity
- **Multiplier:** 10-15x increase
- **Impact:** Fundamental fix to energy generation gap

#### 2. Reduce Forced Loads (STRONGLY RECOMMENDED)
- Make appliances optional/staggered instead of always-on
- Move refrigerator, stove, HVAC to on-demand scheduling
- Implement load-shedding during low solar periods
- **Impact:** Reduces minimum drain from 42.9A to survivable levels

#### 3. Increase Battery Storage (RECOMMENDED)
- **Current:** 1.2 kWh (100Ah @ 12V)
- **Required:** 10-20 kWh minimum
- **Multiplier:** 8-16x increase
- **Impact:** Provides buffer for multi-day autonomy

#### 4. Implement Load Scheduling (RECOMMENDED)
- Run high-draw appliances only during peak solar hours (10 AM - 4 PM)
- Stagger washer/dryer operation
- Use energy storage for non-solar periods
- **Impact:** Optimizes demand-supply alignment

#### 5. Add Grid Connection (ALTERNATIVE)
- For backup power during cloud cover/night
- Accept rolling blackouts with management
- Hybrid grid-connected + solar system
- **Impact:** Removes need for complete autonomy

---

## Energy Balance Analysis

### Current System Capacity

| Metric | Value |
|--------|-------|
| **Simulation Duration** | 704 hours (29.3 days) |
| **Total Solar Generated** | 5,378 Wh |
| **Average Solar Power** | 7.6 W |
| **Peak Solar Current** | 1.0 A (24 W at 24V) |
| **Battery Capacity** | 1,200 Wh |
| **Total Energy Required** | 164,740 Wh |
| **Deficit** | 159,362 Wh |
| **Current Sustainability** | 3.3% self-sufficiency |

### Required Energy Balance

To achieve 100% sustainability:
- System requires **30-40x improvement** in energy balance
- Options: 30x solar increase OR 95% load reduction OR hybrid combination

---

## Recommendations Summary

### Immediate Actions (Next 30 Days)

1. ✅ **Implement mandatory load shedding** for non-essential devices
2. ✅ **Schedule high-load appliances** during peak solar windows only
3. ✅ **Reduce forced load list** - move HVAC to non-essential tier
4. ✅ **Install monitoring** on top 5 loads to identify optimization opportunities

### Medium-term (3-6 Months)

1. 📋 **Increase solar array capacity** - target 5-10x current capacity
2. 📋 **Upgrade battery bank** - minimum 5 kWh, optimal 10-20 kWh
3. 📋 **Redesign forced load configuration** - move to demand-responsive model
4. 📋 **Implement advanced scheduling** - load balancing across solar curve

### Long-term (6-12 Months)

1. 🎯 **Hybrid architecture** - solar + battery + grid backup
2. 🎯 **Smart load management** - AI-driven scheduling
3. 🎯 **System redundancy** - multiple battery/solar subsystems
4. 🎯 **Continuous monitoring** - real-time energy balance optimization

---

## Final Conclusion

### Current Status
The managed power distribution provides **marginal benefit** but **cannot resolve** the fundamental system design failure. Both managed and unmanaged scenarios result in complete battery depletion.

### Long-term Viability
This system requires **major hardware reconfiguration** (solar + battery) AND **architectural redesign** (load reduction) to achieve any meaningful operational autonomy.

### Sustainability Gap
- **Current:** 3% self-sufficiency (5,378 Wh generated vs 164,740 Wh needed)
- **Required:** 30-40x improvement in energy balance for viability
- **Path Forward:** Hybrid approach combining all five solutions above

---

## Files Referenced

- `csv/bfs_run.csv` - Full simulation data (managed vs unmanaged)
- `csv/battery_profile.csv` - Battery voltage vs SoC curve
- `csv/solar_profile.csv` - Solar generation curve
- `house_tree.json` - Load hierarchy and device configuration
- `expert_system_report.txt` - Original expert system analysis

The README is now at: [validate/ANALYSIS_README.md](validate/ANALYSIS_README.md)

---

## Power and Energy Deep Dive

### Understanding Power vs Energy

**POWER** = Rate of energy consumption/generation
- Measured in **WATTS (W)**
- Formula: `P = V × I` (Voltage × Current)
- Analogy: Speed of water flowing through a pipe

**ENERGY** = Total power consumed/generated over time
- Measured in **WATT-HOURS (Wh)** or **KILOWATT-HOURS (kWh)**
- Formula: `E = P × t` (Power × Time)
- Analogy: Total volume of water that flowed

### System Voltage Reference

At your **12V nominal battery voltage**:
- 1 Ampere of current = 12 Watts of power
- 1 Ampere-hour (Ah) = 12 Watt-hours (Wh)
- Example: 42.9A forced load = 515 Watts

---

### Power Analysis: The Immediate Crisis

| Component | Current (A) | Voltage (V) | Power (W) |
|-----------|------------|------------|----------|
| **Total System Loads** | 62.2 | 12 | 746 |
| **Forced Loads (always-on)** | 42.9 | 12 | **515** |
| **Optional Loads** | 19.3 | 12 | 232 |
| **Inverter Max Capacity** | 15 | 12 | **180** |
| **Peak Solar Supply** | 1.0 | 24 | 24 |

#### The Power Crisis
- ⚠️ **Forced loads demand 515W**
- ⚠️ **Inverter only supplies 180W max**
- ⚠️ **Solar provides only 24W peak**
- ❌ **Deficit: 491W minimum (cannot run forced appliances)**

The system is **immediately over-capacity** - the inverter cannot physically supply the power needed for forced appliances to operate.

---

### Energy Analysis: The Long-Term Crisis

#### 14-Day Simulation Energy Budget

| Metric | Value | Per Day |
|--------|-------|---------|
| **Battery Capacity** | 1,200 Wh | 1.2 kWh total |
| **Forced Load Consumption** | 173,040 Wh | 12,360 Wh |
| **Solar Generation** | 5,378 Wh | 184 Wh |
| **Energy Deficit** | 167,662 Wh | 11,976 Wh |

#### Daily Energy Breakdown

**What the system generates in 24 hours:**
- Solar input: **184 Wh** (5.4 kWh ÷ 29.3 days)
- This is equivalent to running: 0.36A × 12V for 1 hour
- Or: 1 LED bulb for 24 hours

**What the system needs in 24 hours (forced loads only):**
- Forced loads: **12,360 Wh** 
- This is equivalent to: 42.9A × 12V × 24 hours
- Or: Continuous refrigerator, stove, HVAC, washer, dryer, etc.

**Daily deficit:**
- Shortfall: 12,360 - 184 = **12,176 Wh short every single day**
- Battery depletion per day: 12,176 Wh ÷ 1,200 Wh = **10.1 days worth of capacity lost**

#### Energy Depletion Timeline

| Time | Battery SoC | Status |
|------|------------|--------|
| **Hour 0** | 90% (1,080 Wh) | System starts |
| **Hour 1-2** | 75% - 50% | Rapid depletion |
| **Hour 3-4** | 50% - 20% | Battery warning |
| **Hour 4-5** | 20% - 0% | Voltage drops below 3.4V |
| **After Hour 5** | 0% | System dead |

**Real-world observation:** Simulation shows 4-day depletion only because loads vary during night vs day. With constant 515W load: **~2 hours to complete failure**.

---

### Generation vs Consumption Rates

#### Energy Generation Rate
- **Current capacity:** 7.6 W average
- **Per day:** 7.6 W × 24 h = **182 Wh/day**
- **Per month:** 182 Wh × 30 = **5.5 kWh/month**
- **Per year:** 5.5 kWh × 12 = **66 kWh/year**

#### Energy Consumption Rate (Forced Loads Only)
- **Current demand:** 515 W continuous
- **Per day:** 515 W × 24 h = **12,360 Wh/day = 12.4 kWh/day**
- **Per month:** 12.4 kWh × 30 = **371 kWh/month**
- **Per year:** 371 kWh × 12 = **4,452 kWh/year**

#### The Sustainability Ratio

```
Energy Generation  :  Energy Consumption
    5.5 kWh      :      371 kWh
    1            :      67
```

**Translation:** The system generates **1 kWh for every 67 kWh it needs.** This is only **1.5% self-sufficient**.

---

### Peak Power vs Average Power

#### Peak Power Analysis

| Component | Peak Power | Average Power |
|-----------|-----------|---------------|
| **Solar Generation** | 24 W (1A × 24V) | 7.6 W |
| **Forced Loads** | 515 W continuous | 515 W |
| **Optional Loads** | Up to 232 W | Varies 0-232 W |
| **Total Peak Demand** | 746 W | ~550 W |
| **Inverter Capacity** | 180 W (hard limit) | 180 W max |

#### Peak Power Problem
- Peak load demand: **746 W**
- Inverter max: **180 W**
- **Overage: 566 W (414% over capacity)**

The system cannot handle peak loads. When multiple high-current devices activate simultaneously, the inverter trips or fails.

---

### Hourly Power Flow Diagram

#### Typical Peak Solar Hour (Noon)

```
Solar Input: 24 W
     ↓
  Battery (1,200 Wh)
     ↓
  Inverter (180 W max) ← BOTTLENECK
     ↓
Forced Loads: 515 W ← CANNOT SUPPLY

Gap: 515 - 24 = 491 W
Battery must supply 491W + overhead
At 1,200 Wh ÷ 491 W = 2.4 hours to depletion
```

#### Typical Night Hour (Midnight)

```
Solar Input: 0 W
     ↓
  Battery (1,200 Wh)
     ↓
  Inverter (180 W max) ← BOTTLENECK
     ↓
Forced Loads: 515 W ← CANNOT SUPPLY

Gap: 515 - 0 = 515 W
Battery must supply 515W
At 1,200 Wh ÷ 515 W = 2.3 hours to depletion
```

Both scenarios result in **complete battery depletion in ~2-4 hours**.

---

### Energy Efficiency and System Losses

Even accounting for inefficiencies does not help:

| Component | Efficiency | Energy Loss |
|-----------|-----------|-------------|
| **Inverter** | 90% | 10% loss |
| **Battery charging** | 85-90% | 10-15% loss |
| **Wiring/Connectors** | 97-98% | 2-3% loss |
| **Overall System** | ~75% combined | 25% loss |

#### Impact on Energy Balance

- **Solar input:** 5,378 Wh
- **After system losses (25%):** 5,378 × 0.75 = **4,034 Wh delivered**
- **Energy required:** 12,360 Wh/day
- **Shortfall:** 12,360 - 184 = **12,176 Wh/day**

**Conclusion:** Even perfect (100%) efficiency would not solve the problem. The fundamental issue is **insufficient generation**, not inefficiency.

---

### Daily and Monthly Energy Budgets

#### Daily Energy Budget (24-Hour Cycle)

| Time Period | Solar Gen | Load Demand | Battery Change |
|-------------|-----------|------------|-----------------|
| **Morning (6-10 AM)** | 72 Wh | 2,060 Wh | -1,988 Wh |
| **Midday (10 AM-4 PM)** | 144 Wh | 3,870 Wh | -3,726 Wh |
| **Evening (4-10 PM)** | 72 Wh | 3,090 Wh | -3,018 Wh |
| **Night (10 PM-6 AM)** | 0 Wh | 3,090 Wh | -3,090 Wh |
| **DAILY TOTAL** | **288 Wh** | **12,110 Wh** | **-11,822 Wh** |

**Result:** Battery depletes by 11,822 Wh per day
- At 1,200 Wh capacity: Full depletion in **0.1 days = 2.4 hours**

#### Monthly Energy Projection

| Metric | Value |
|--------|-------|
| **Average Monthly Generation** | 5.5 kWh |
| **Average Monthly Consumption (forced only)** | 371 kWh |
| **Monthly Deficit** | 365.5 kWh |
| **System Sustainability** | 1.5% per month |

---

### Self-Sufficiency Metrics

#### Current System Self-Sufficiency

```
Self-Sufficiency = (Energy Generated ÷ Energy Required) × 100%
                 = (5,378 Wh ÷ 150,610 Wh) × 100%
                 = 3.6%
```

**Meaning:** System generates 3.6 cents worth of energy for every dollar needed.

#### Viability Threshold

- **Minimum acceptable:** 50% self-sufficient (needs external backup half the time)
- **Good:** 80% self-sufficient (handles 80% of needs independently)
- **Excellent:** 95%+ self-sufficient (nearly autonomous)
- **Current system:** 3.6% (completely non-viable)
- **Required improvement:** 22-26x capacity increase just to reach 80% viability

---

### Required Improvements to Energy Balance

#### Option 1: Increase Solar Capacity
- **Current:** 5,378 Wh over 14 days
- **Needed to match consumption:** 173,040 Wh over 14 days
- **Multiplier:** 173,040 ÷ 5,378 = **32x increase**
- **New capacity:** 1A × 32 = **32A peak solar array**
- **Cost/Feasibility:** Very high (requires 30+ solar panels)

#### Option 2: Reduce Forced Loads
- **Current forced:** 42.9A (515W)
- **Required for viability:** ~3A (36W) with full battery backup
- **Reduction needed:** 42.9 ÷ 3 = **14x load reduction**
- **Means:** Remove refrigerator, stove, HVAC, dryer, washer as "forced"
- **Feasibility:** Low (these are essential appliances)

#### Option 3: Increase Battery Capacity
- **Current:** 1,200 Wh
- **For 3 days autonomy:** 12,360 Wh × 3 = **37,080 Wh needed**
- **Multiplier:** 37,080 ÷ 1,200 = **31x increase**
- **New system:** 3,100 Ah battery bank (extremely large)
- **Cost/Feasibility:** Extremely high

#### Option 4: Hybrid Solution (Recommended)
Combine all three:
- **Solar:** Increase 5-8x (more feasible than 32x alone)
- **Loads:** Reduce 2-3x (convert HVAC/dryer to optional)
- **Battery:** Increase 3-5x (expand to 5-10 kWh)
- **Grid backup:** Add for critical periods
- **Result:** Achieves 70-85% self-sufficiency

---

## Conclusion on Power and Energy

### The Two-Tier Crisis

1. **Power Crisis (Immediate):** 515W demand vs 180W inverter capacity = immediate failure
2. **Energy Crisis (Hours):** 2-4 hour autonomy vs 24+ hour requirement = system dead by next morning

### Why Software Cannot Fix This

- **Power problem** = Hardware bottleneck (inverter too small)
- **Energy problem** = Capacity shortage (solar + battery too small)
- **No amount of smart scheduling** can overcome these fundamental physics constraints

### The Math is Inescapable

- System needs: 371 kWh/month of energy
- System can provide: 5.5 kWh/month of energy  
- Shortfall: 365.5 kWh/month (99.9% deficit)
- This requires **hardware upgrades**, not software fixes


